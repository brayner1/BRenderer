#include "SceneRenderer.h"

#include <ranges>

#include <Renderer/Storages/RenderStorageGlobals.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Core/LogSystem.h>
#include <Core/Assets/AssetManager.h>

#include "Internal/IdOwner.h"

#define MAX_LIGHTS 256

struct CameraUniform
{
    glm::mat4 projection_view{1.f};
};

struct Transform3DUniform
{
    glm::mat4 model_matrix;
};

constexpr uint32_t scene_descriptor_set_index    = 0;
constexpr uint32_t model_descriptor_set_index    = 1;
constexpr uint32_t material_descriptor_set_index = 2;

namespace brr::render
{
    static internal::IdOwner<uint32_t> s_viewport_id_owner;

    SceneRenderer::SceneRenderer()
        : m_render_device(VKRD::GetSingleton())
    {
        assert(m_render_device && "Can't create SceneRenderer without VulkanRenderDevice.");
        BRR_LogInfo("Creating SceneRenderer");

        {
            m_shader_id = RenderStorageGlobals::material_storage.GetDefaultShaderID();
            Shader* default_shader = RenderStorageGlobals::material_storage.GetShader(m_shader_id);
            assert(default_shader != nullptr && "Default shader must be initialized when constructing SceneRenderer.");

            m_graphics_pipeline = m_render_device->Create_GraphicsPipeline(*default_shader, {DataFormat::R8G8B8A8_SRGB},
                                                                           DataFormat::D32_Float);
        }

        SetupSceneUniforms();

        m_image = AssetManager::GetOrCreateAsset<vis::Image>("Resources/UV_Grid.png");

        TextureID texture_id = m_image->GetTextureID();
        TextureStorage::Texture* texture = RenderStorageGlobals::texture_storage.GetTexture(texture_id);
        m_texture_2d_handle = texture->texture_2d_handle;

        MaterialProperties  default_material_properties;
        default_material_properties.diffuse_texture = texture_id;

        m_default_material = RenderStorageGlobals::material_storage.AllocateResource();
        RenderStorageGlobals::material_storage.InitMaterial(m_default_material, default_material_properties, m_shader_id);
    }

    SceneRenderer::~SceneRenderer()
    {
        m_render_device->WaitIdle();
        m_render_device->DestroyGraphicsPipeline(m_graphics_pipeline);
        RenderStorageGlobals::material_storage.DestroyMaterial(m_default_material);
        m_render_device->DestroyTexture2D(m_texture_2d_handle);
    }

    void SceneRenderer::CreateCamera(CameraID camera_id,
                                     EntityID owner_entity,
                                     float camera_fovy,
                                     float camera_near,
                                     float camera_far)
    {
        m_camera_id = camera_id;
        CameraInfo new_camera{
            .camera_fov_y = camera_fovy,
            .camera_near = camera_near,
            .camera_far = camera_far,
            .owner_entity = owner_entity
        };
        m_cameras.AddObject(camera_id, std::move(new_camera));
        BRR_LogInfo("Initialized Entity Uniform Buffers.");
    }

    void SceneRenderer::DestroyCamera(CameraID camera_id)
    {
        if (!m_cameras.Contains(camera_id))
        {
            BRR_LogError("Can't destroy SceneRenderer Camera (ID: {}) because this Camera doesn't exist.",
                         static_cast<uint32_t>(camera_id));
            return;
        }

        m_cameras.RemoveObject(camera_id);
        // TODO: Should this be optmized? Probably not a common operation.
        for (Viewport& viewport : m_viewports)
        {
            if (viewport.camera_id == camera_id)
            {
                viewport.camera_uniform_dirty.fill(true);
            }
        }
    }

    void SceneRenderer::UpdateCameraProjection(CameraID camera_id,
                                               float camera_fovy,
                                               float camera_near,
                                               float camera_far)
    {
        CameraInfo& camera_info  = m_cameras.Get(camera_id);
        camera_info.camera_fov_y = camera_fovy;
        camera_info.camera_near  = camera_near;
        camera_info.camera_far   = camera_far;

        // TODO: Should this be optmized? Probably not a common operation.
        for (Viewport& viewport : m_viewports)
        {
            if (viewport.camera_id == camera_id)
            {
                viewport.camera_uniform_dirty.fill(true);
            }
        }
    }

    void SceneRenderer::CreateEntity(EntityID entity_id,
                                     const glm::mat4& entity_transform)
    {
        if (m_entities_map.contains(entity_id))
        {
            BRR_LogError("Can't create SceneRenderer Entity (ID: {}) because this entity already exists.",
                         static_cast<uint32_t>(entity_id));
            return;
        }

        auto [entity_info_it, success] = m_entities_map.emplace(entity_id, EntityInfo{});

        entity_info_it->second.current_matrix = entity_transform;
        SetupEntityUniforms(entity_info_it->second);
    }

    void SceneRenderer::DestroyEntity(EntityID entity_id)
    {
        if (!m_entities_map.contains(entity_id))
        {
            BRR_LogError("Can't destroy SceneRenderer Entity (ID: {}) because this entity doesn't exist.",
                         static_cast<uint32_t>(entity_id));
            return;
        }

        auto entity_node = m_entities_map.extract(entity_id);

        for (DescriptorSetHandle& descriptor_set_handle : entity_node.mapped().descriptor_sets)
        {
            m_render_device->DescriptorSet_Destroy(descriptor_set_handle);
        }

        // Update cached surfaces and materials.
        for (SurfaceID surface_id : entity_node.mapped().surfaces)
        {
            if (m_cached_surfaces.Contains(surface_id))
            {
                // Erase entity from surface owner nodes.
                SurfaceRenderData& surface_cached_data = m_cached_surfaces.Get(surface_id);
                std::erase(surface_cached_data.m_owner_nodes, entity_id);

                // If surface has no more owner nodes, remove it from cached surfaces.
                if (surface_cached_data.m_owner_nodes.empty())
                {
                    DereferenceMaterial(surface_cached_data.m_material_id);
                    m_cached_surfaces.RemoveObject(surface_id);
                }
            }
        }
    }

    void SceneRenderer::UpdateEntityTransform(EntityID entity_id,
                                              const glm::mat4& entity_transform)
    {
        auto entity_it = m_entities_map.find(entity_id);
        if (entity_it == m_entities_map.end())
        {
            BRR_LogError("Can't update SceneRenderer Entity (ID: {}) transform because this entity doesn't exist.",
                         static_cast<uint32_t>(entity_id));
            return;
        }

        // Change current transform and signal uniforms as dirty.
        entity_it->second.current_matrix = entity_transform;
        MarkEntityDirty(entity_id, entity_it->second, false, true);
    }

    void SceneRenderer::AppendSurfaceToEntity(SurfaceID surface_id,
                                              EntityID owner_entity)
    {
        auto entity_it = m_entities_map.find(owner_entity);
        if (entity_it == m_entities_map.end())
        {
            BRR_LogError("Can't append Surface (ID: {}) to SceneRenderer Entity (ID: {}) because this entity doesn't exist.",
                         static_cast<uint64_t>(surface_id), static_cast<uint32_t>(owner_entity));
            return;
        }

        RenderSurface* render_surface = RenderStorageGlobals::mesh_storage.GetSurface(surface_id);
        if (!render_surface)
        {
            BRR_LogError("Can't append Surface (ID: {}) to SceneRenderer Entity (ID: {}) because this Surface doesn't exist in renderer surface storage.",
                         static_cast<uint64_t>(surface_id), static_cast<uint32_t>(owner_entity));
            return;
        }

        EntityInfo& entity_info = entity_it->second;
        if (std::find(entity_info.surfaces.begin(), entity_info.surfaces.end(), surface_id) != entity_info.surfaces.end())
        {
            BRR_LogError("Can't append Surface (ID: {}) to SceneRenderer Entity (ID: {}) because this Surface is already assigned to this entity.",
                         static_cast<uint64_t>(surface_id), static_cast<uint32_t>(owner_entity));
            return;
        }

        entity_info.surfaces.push_back(surface_id);
        MarkEntityDirty(owner_entity, entity_info, true, false);
        BRR_LogInfo("Appended Surface (ID: {}) to Entity (ID: {}).", static_cast<uint64_t>(surface_id), static_cast<uint32_t>(owner_entity));

        MaterialID surface_material_id = render_surface->m_material_id.IsValid() ?
                render_surface->m_material_id : m_default_material;
        if (m_cached_surfaces.Contains(surface_id))
        {
            SurfaceRenderData& render_data = m_cached_surfaces.Get(surface_id);
            render_data.m_owner_nodes.push_back(owner_entity);
        }
        else
        {
            SurfaceRenderData render_data (owner_entity);
            // Surface Data
            render_data.m_surface_id = surface_id;
            render_data.m_vertex_buffer_handle = render_surface->m_vertex_buffer;
            render_data.m_index_buffer_handle = render_surface->m_index_buffer;
            render_data.m_num_vertices = render_surface->num_vertices;
            render_data.m_num_indices = render_surface->num_indices;

            // Material Data
            render_data.m_material_id = surface_material_id;

            m_cached_surfaces.AddObject(surface_id, render_data);
            ReferenceNewMaterial(surface_material_id);
        }
    }

    void SceneRenderer::NotifySurfaceChanged(SurfaceID surface_id)
    {
        if (m_cached_surfaces.Contains(surface_id))
        {
            SurfaceRenderData& surface_cached_data = m_cached_surfaces.Get(surface_id);
            surface_cached_data.m_surface_dirty = true;

            RenderSurface* render_surface = RenderStorageGlobals::mesh_storage.GetSurface(surface_id);
            bool is_removed = render_surface == nullptr;

            // Mark entities surfaces as dirty. Delete SurfaceID from entity surfaces if surface was removed.
            for (auto& owner_node : surface_cached_data.m_owner_nodes)
            {
                auto entity_it = m_entities_map.find(owner_node);
                if (entity_it != m_entities_map.end())
                {
                    EntityInfo& entity_info = entity_it->second;
                    MarkEntityDirty(owner_node, entity_info, true, false);
                    if (is_removed)
                    {
                        std::erase(entity_info.surfaces, surface_id);
                    }
                }
            }

            // Update surface cached data or remove it from cached data if it was removed.
            if (!is_removed)
            {
                surface_cached_data.m_vertex_buffer_handle = render_surface->m_vertex_buffer;
                surface_cached_data.m_index_buffer_handle = render_surface->m_index_buffer;
                surface_cached_data.m_num_vertices = render_surface->num_vertices;
                surface_cached_data.m_num_indices = render_surface->num_indices;
                // TODO: Handle material change.
                if (surface_cached_data.m_material_id != render_surface->m_material_id)
                {
                    MaterialID new_material_id = render_surface->m_material_id.IsValid() ?
                            render_surface->m_material_id : m_default_material;
                    // Decrease old material reference count, and erase it from cached materials if no more references.
                    DereferenceMaterial(surface_cached_data.m_material_id);
                    // Increase new material reference count, and add it to cached materials if it doesn't exist.
                    ReferenceNewMaterial(new_material_id);
                    surface_cached_data.m_material_id = new_material_id;
                }
            }
            else
            {
                DereferenceMaterial(surface_cached_data.m_material_id);
                m_cached_surfaces.RemoveObject(surface_id);
            }
        }
    }

    void SceneRenderer::CreatePointLight(LightID light_id,
                                         const glm::vec3& position,
                                         const glm::vec3& color,
                                         float intensity)
    {
        Light point_light;
        point_light.light_type      = 0;
        point_light.light_position  = position;
        point_light.light_intensity = intensity;
        point_light.light_color     = color;
        CreateNewLight(light_id, std::move(point_light));

        BRR_LogInfo("Created new PointLight. LightID: {}", static_cast<uint32_t>(light_id));
    }

    void SceneRenderer::UpdatePointLight(LightID light_id,
                                         const glm::vec3& position,
                                         const glm::vec3& color,
                                         float intensity)
    {
        Light& point_light          = m_scene_lights.Get(light_id);
        point_light.light_position  = position;
        point_light.light_color     = color;
        point_light.light_intensity = intensity;

        m_scene_uniform_info.m_light_storage_dirty.fill(true);
    }

    void SceneRenderer::CreateDirectionalLight(LightID light_id,
                                               const glm::vec3& direction,
                                               const glm::vec3& color,
                                               float intensity)
    {
        Light directional_light;
        directional_light.light_type      = 1;
        directional_light.light_intensity = intensity;
        directional_light.light_direction = direction;
        directional_light.light_color     = color;
        CreateNewLight(light_id, std::move(directional_light));

        BRR_LogInfo("Created new DirectionalLight. LightID: {}", static_cast<uint32_t>(light_id));
    }

    void SceneRenderer::UpdateDirectionalLight(LightID light_id,
                                               const glm::vec3& direction,
                                               const glm::vec3& color,
                                               float intensity)
    {
        Light& directional_light          = m_scene_lights.Get(light_id);
        directional_light.light_intensity = intensity;
        directional_light.light_direction = direction;
        directional_light.light_color     = color;

        m_scene_uniform_info.m_light_storage_dirty.fill(true);
    }

    void SceneRenderer::CreateSpotLight(LightID light_id,
                                           const glm::vec3& position,
                                           float cutoff_angle,
                                           const glm::vec3& direction,
                                           float intensity,
                                           const glm::vec3& color)
    {
        Light spot_light;
        spot_light.light_type      = 2;
        spot_light.light_position  = position;
        spot_light.light_intensity = intensity;
        spot_light.light_direction = direction;
        spot_light.light_color     = color;
        spot_light.light_cutoff    = std::cos(cutoff_angle);
        CreateNewLight(light_id, std::move(spot_light));

        BRR_LogInfo("Created new SpotLight. LightID: {}", static_cast<uint32_t>(light_id));
    }

    void SceneRenderer::UpdateSpotLight(LightID light_id,
                                        const glm::vec3& position,
                                        float cutoff_angle,
                                        const glm::vec3& direction,
                                        float intensity,
                                        const glm::vec3& color)
    {
        Light& spot_light          = m_scene_lights.Get(light_id);
        spot_light.light_position  = position;
        spot_light.light_intensity = intensity;
        spot_light.light_direction = direction;
        spot_light.light_color     = color;
        spot_light.light_cutoff    = std::cos(cutoff_angle);

        m_scene_uniform_info.m_light_storage_dirty.fill(true);
    }

    void SceneRenderer::CreateAmbientLight(LightID light_id,
                                           const glm::vec3& color,
                                           float intensity)
    {
        Light ambient_light;
        ambient_light.light_type      = 3;
        ambient_light.light_intensity = intensity;
        ambient_light.light_color     = color;
        CreateNewLight(light_id, std::move(ambient_light));

        BRR_LogInfo("Created new AmbientLight. LightID: {}", static_cast<uint32_t>(light_id));
    }

    void SceneRenderer::UpdateAmbientLight(LightID light_id,
                                           const glm::vec3& color,
                                           float intensity)
    {
        Light& ambient_light          = m_scene_lights.Get(light_id);
        ambient_light.light_intensity = intensity;
        ambient_light.light_color     = color;

        m_scene_uniform_info.m_light_storage_dirty.fill(true);
    }

    void SceneRenderer::DestroyLight(LightID light_id)
    {
        m_scene_lights.RemoveObject(light_id);

        m_scene_uniform_info.m_light_storage_dirty.fill(true);
        m_scene_uniform_info.m_light_storage_size_changed.fill(true);

        BRR_LogInfo("Removed Light. LightID: {}", static_cast<uint32_t>(light_id));
    }

    ViewportID SceneRenderer::CreateViewport(glm::uvec2 viewport_size, CameraID camera_id)
    {
        Viewport viewport{.width = viewport_size.x, .height = viewport_size.y, .camera_id = camera_id};
        for (uint32_t idx = 0; idx < FRAME_LAG; idx++)
        {
            viewport.color_attachment[idx] = m_render_device->Create_Texture2D(viewport.width, viewport.height,
                                                                               ImageUsage::ColorAttachmentImage |
                                                                               ImageUsage::TransferSrcImage,
                                                                               DataFormat::R8G8B8A8_SRGB);
            viewport.depth_attachment[idx] = m_render_device->Create_Texture2D(viewport.width, viewport.height,
                                                                               ImageUsage::DepthStencilAttachmentImage,
                                                                               DataFormat::D32_Float);
        }
        ViewportID new_viewport_id = static_cast<ViewportID>(s_viewport_id_owner.GetNewId());
        m_viewports.AddObject(new_viewport_id, std::move(viewport));

        Viewport& new_viewport = m_viewports.Get(new_viewport_id);
        SetupViewportUniforms(new_viewport);

        BRR_LogInfo("Created Viewport. Viewport ID: {}. Viewport Size: (width: {}, height: {})", static_cast<uint32_t>(new_viewport_id), viewport_size.x, viewport_size.y);
        return new_viewport_id;
    }

    void SceneRenderer::ResizeViewport(ViewportID viewport_id,
                                       glm::uvec2 new_size)
    {
        if (!m_viewports.Contains(viewport_id))
        {
            BRR_LogError("Resizing Viewport (ID: {}) that does not exist in this SceneRenderer.", uint32_t(viewport_id));
            return;
        }
        Viewport& viewport = m_viewports.Get(viewport_id);
        viewport.width     = new_size.x;
        viewport.height    = new_size.y;
        for (uint32_t idx = 0; idx < FRAME_LAG; idx++)
        {
            m_render_device->DestroyTexture2D(viewport.color_attachment[idx]);
            viewport.color_attachment[idx] = m_render_device->Create_Texture2D(viewport.width, viewport.height,
                                                                               ImageUsage::ColorAttachmentImage |
                                                                               ImageUsage::TransferSrcImage,
                                                                               DataFormat::R8G8B8A8_SRGB);

            m_render_device->DestroyTexture2D(viewport.depth_attachment[idx]);
            viewport.depth_attachment[idx] = m_render_device->Create_Texture2D(viewport.width, viewport.height,
                                                                               ImageUsage::DepthStencilAttachmentImage,
                                                                               DataFormat::D32_Float);
        }

        viewport.camera_uniform_dirty.fill(true);
        BRR_LogInfo("Resized Viewport. Viewport ID: {}. Viewport New Size: (width: {}, height: {})", static_cast<uint32_t>(viewport_id), new_size.x, new_size.y);
    }

    void SceneRenderer::DestroyViewport(ViewportID viewport_id)
    {
        if (!m_viewports.Contains(viewport_id))
        {
            BRR_LogError("Destroying Viewport (ID: {}) that does not exist in this SceneRenderer.", uint32_t(viewport_id));
            return;
        }
        Viewport& viewport = m_viewports.Get(viewport_id);
        for (uint32_t idx = 0; idx < FRAME_LAG; idx++)
        {
            m_render_device->DestroyTexture2D(viewport.color_attachment[idx]);
            m_render_device->DestroyTexture2D(viewport.depth_attachment[idx]);
        }

        m_viewports.RemoveObject(viewport_id);
        BRR_LogInfo("Destroyed Viewport. Viewport ID: {}", static_cast<uint32_t>(viewport_id));
    }

    CameraID SceneRenderer::GetViewportCameraID(ViewportID viewport_id) const
    {
        if (!m_viewports.Contains(viewport_id))
        {
            BRR_LogError("Getting CameraID from Viewport (ID: {}) that does not exist in this SceneRenderer.", uint32_t(viewport_id));
            return CameraID::NULL_ID;
        }
        const Viewport& viewport = m_viewports.Get(viewport_id);
        return viewport.camera_id;
    }

    void SceneRenderer::SetViewportCameraID(ViewportID viewport_id,
                                            CameraID camera_id)
    {
        if (!m_viewports.Contains(viewport_id))
        {
            BRR_LogError("Setting CameraID on Viewport (ID: {}) that does not exist in this SceneRenderer.", uint32_t(viewport_id));
            return;
        }
        Viewport& viewport = m_viewports.Get(viewport_id);
        if (viewport.camera_id != camera_id)
        {
            viewport.camera_id = camera_id;
            viewport.camera_uniform_dirty.fill(true);
        }
    }

    void SceneRenderer::UpdateDirtyInstances()
    {
        assert(m_render_device && "VulkanRenderDevice must be initialized on construction.");
        if (m_current_frame == m_render_device->GetCurrentFrameNumber())
        {
            return;
        }
        m_current_frame  = m_render_device->GetCurrentFrameNumber();
        m_current_buffer = m_render_device->GetCurrentFrameBufferIndex();

        // Update dirty
        std::set<EntityID> updated_entities;
        std::vector<EntityID> entities_to_remove;
        for (auto& entity_id : m_dirty_entities)
        {
            EntityInfo& entity = m_entities_map.at(entity_id);
            if (entity.uniform_dirty[m_current_buffer])
            {
                entity.uniform_buffers[m_current_buffer].Map();
                entity.uniform_buffers[m_current_buffer].WriteToBuffer(
                    &entity.current_matrix, sizeof(glm::mat4));
                entity.uniform_buffers[m_current_buffer].Unmap();

                entity.uniform_dirty[m_current_buffer] = false;
                updated_entities.emplace(entity_id);
            }

            if (entity.surfaces_dirty)
            {
                // TODO: maintain AABB updated.
                entity.surfaces_dirty = false;
            }

            if (!std::any_of(entity.uniform_dirty.begin(), entity.uniform_dirty.end(), [](bool value) {return value==true;}))
            {
                entities_to_remove.push_back(entity_id);
            }
        }

        for (auto& entity : entities_to_remove)
        {
            m_dirty_entities.erase(entity);
        }

        for (Viewport& viewport : m_viewports)
        {
            if (!m_cameras.Contains(viewport.camera_id))
            {
                BRR_LogError("Error: Viewport (ID: {}) has Camera (ID: {}) assigned, but this Camera is not initialized.\nSkipping viewport camera update.");
                continue; 
            }
            CameraInfo& camera_info = m_cameras.Get(viewport.camera_id);
            if (viewport.camera_uniform_dirty[m_current_buffer] || updated_entities.contains(camera_info.owner_entity))
            {
                float aspect                = (float)viewport.width / (float)viewport.height;
                glm::mat4 projection_matrix = glm::perspective(camera_info.camera_fov_y, aspect, camera_info.camera_near,
                                                               camera_info.camera_far);

                EntityInfo& entity    = m_entities_map[camera_info.owner_entity];
                glm::mat4 view_matrix = glm::inverse(entity.current_matrix);

                CameraUniform camera_uniform;
                camera_uniform.projection_view = projection_matrix * view_matrix;
                viewport.camera_uniform_buffers[m_current_buffer].Map();
                viewport.camera_uniform_buffers[m_current_buffer].WriteToBuffer(
                    &camera_uniform, sizeof(CameraUniform));
                viewport.camera_uniform_buffers[m_current_buffer].Unmap();
                viewport.camera_uniform_dirty[m_current_buffer] = false;
            }
        }
        

        if (m_scene_uniform_info.m_light_storage_dirty[m_current_buffer])
        {
            m_scene_uniform_info.m_light_storage_dirty[m_current_buffer] = false;

            m_scene_uniform_info.m_lights_buffers[m_current_buffer].Map();
            m_scene_uniform_info.m_lights_buffers[m_current_buffer].WriteToBuffer(
                m_scene_lights.Data(), m_scene_lights.Size() * sizeof(Light));
            m_scene_uniform_info.m_lights_buffers[m_current_buffer].Unmap();

            if (m_scene_uniform_info.m_light_storage_size_changed[m_current_buffer])
            {
                m_scene_uniform_info.m_light_storage_size_changed[m_current_buffer] = false;

                Shader* shader = RenderStorageGlobals::material_storage.GetShader(m_shader_id);
                assert(shader != nullptr && "Default shader must be initialized when updating SceneRenderer.");

                DescriptorLayout layout = shader->GetDescriptorSetLayouts()[0];
                auto setBuilder         = DescriptorSetUpdater(layout);
                setBuilder.BindBuffer(0, m_scene_uniform_info.m_lights_buffers[m_current_buffer].GetHandle(),
                                      m_scene_lights.Size() * sizeof(Light));

                setBuilder.UpdateDescriptorSet(m_scene_uniform_info.m_descriptor_sets[m_current_buffer]);
            }
        }
    }

    void SceneRenderer::Render3D(ViewportID viewport_id,
                                 Texture2DHandle render_target)
    {
        Viewport& viewport = m_viewports.Get(viewport_id);

        m_render_device->RenderTarget_BeginRendering(viewport.color_attachment[m_current_buffer],
                                                     viewport.depth_attachment[m_current_buffer]);

        m_render_device->Bind_GraphicsPipeline(m_graphics_pipeline);

        // Scene uniform (light array)
        m_render_device->Bind_DescriptorSet(m_graphics_pipeline,
                                            m_scene_uniform_info.m_descriptor_sets[m_current_buffer],
                                            0);
        // Viewport uniform (camera matrix)
        m_render_device->Bind_DescriptorSet(m_graphics_pipeline, viewport.camera_descriptor_sets[m_current_buffer], 1);

        MaterialID last_material_id = MaterialID();
        for (SurfaceRenderData& render_data : m_cached_surfaces)
        {
            // Bind Material uniform, if necessary
            if (last_material_id != render_data.m_material_id)
            {
                MaterialRenderData& material_render_data = m_cached_materials.Get(render_data.m_material_id);
                m_render_device->Bind_DescriptorSet(m_graphics_pipeline, material_render_data.m_material_descriptor_sets[m_current_buffer], 2);
                last_material_id = render_data.m_material_id;
            }

            for (EntityID owner_node : render_data.m_owner_nodes)
            {
                auto entity_iter = m_entities_map.find(owner_node);
                if (entity_iter == m_entities_map.end())
                {
                    BRR_LogError(
                        "Surface (ID: {}) is owned by non-existing Entity (ID: {}).\nSkipping rendering of this Entity.",
                        uint64_t(render_data.m_surface_id), uint32_t(owner_node));
                    continue;
                }
                EntityInfo& entity_info = entity_iter->second;

                // Entity uniform (model matrix)
                m_render_device->Bind_DescriptorSet(m_graphics_pipeline, entity_info.descriptor_sets[m_current_buffer], 3);

                assert(
                    render_data.m_vertex_buffer_handle.IsValid() && "Vertex buffer must be valid to bind to a command buffer.");
                m_render_device->BindVertexBuffer(render_data.m_vertex_buffer_handle);

                if (render_data.m_index_buffer_handle.IsValid())
                {
                    m_render_device->BindIndexBuffer(render_data.m_index_buffer_handle);
                    m_render_device->DrawIndexed(render_data.m_num_indices, 1, 0, 0, 0);
                }
                else
                {
                    m_render_device->Draw(render_data.m_num_vertices, 1, 0, 0);
                }
            }
        }

        m_render_device->RenderTarget_EndRendering(viewport.color_attachment[m_current_buffer]);
        m_render_device->Texture2D_Blit(viewport.color_attachment[m_current_buffer], render_target);
    }

    void SceneRenderer::SetupSceneUniforms()
    {
        for (size_t idx = 0; idx < FRAME_LAG; idx++)
        {
            m_scene_uniform_info.m_lights_buffers[idx] =
                DeviceBuffer(sizeof(Light) * MAX_LIGHTS,
                                     BufferUsage::StorageBuffer | BufferUsage::HostAccessSequencial,
                                     MemoryUsage::AUTO);
        }

        Shader* shader = RenderStorageGlobals::material_storage.GetShader(m_shader_id);
        assert(shader != nullptr && "Default shader must be initialized when updating SceneRenderer.");

        DescriptorLayout layout = shader->GetDescriptorSetLayouts()[0];

        std::vector<DescriptorSetHandle> descriptors_handles = m_render_device->DescriptorSet_Allocate(
            layout.m_layout_handle, FRAME_LAG);
        std::ranges::copy(descriptors_handles, m_scene_uniform_info.m_descriptor_sets.begin());

        for (uint32_t set_idx = 0; set_idx < FRAME_LAG; set_idx++)
        {
            auto setBuilder = DescriptorSetUpdater(layout);
            setBuilder.BindBuffer(0, m_scene_uniform_info.m_lights_buffers[set_idx].GetHandle(), sizeof(Light));

            setBuilder.UpdateDescriptorSet(m_scene_uniform_info.m_descriptor_sets[set_idx]);
        }
    }

    void SceneRenderer::SetupViewportUniforms(Viewport& viewport)
    {
        size_t buffer_size = sizeof(CameraUniform);

        for (uint32_t frame_idx = 0; frame_idx < FRAME_LAG; frame_idx++)
        {
            viewport.camera_uniform_buffers[frame_idx] = DeviceBuffer(buffer_size,
                                                                      UniformBuffer | HostAccessSequencial,
                                                                      MemoryUsage::AUTO);
        }

        Shader* shader = RenderStorageGlobals::material_storage.GetShader(m_shader_id);
        assert(shader != nullptr && "Default shader must be initialized when updating SceneRenderer.");

        // Init viewport camera descriptor sets
        DescriptorLayout descriptor_layout = shader->GetDescriptorSetLayouts()[1];

        std::vector<DescriptorSetHandle> descriptors_handles = m_render_device->
            DescriptorSet_Allocate(descriptor_layout.m_layout_handle,
                                   FRAME_LAG);
        std::ranges::copy(descriptors_handles, viewport.camera_descriptor_sets.begin());

        for (uint32_t frame_idx = 0; frame_idx < FRAME_LAG; frame_idx++)
        {
            auto setBuilder = DescriptorSetUpdater(descriptor_layout);
            setBuilder.BindBuffer(0, viewport.camera_uniform_buffers[frame_idx].GetHandle(), buffer_size);

            setBuilder.UpdateDescriptorSet(viewport.camera_descriptor_sets[frame_idx]);
        }

        CameraUniform camera_uniform;
        if (m_cameras.Contains(viewport.camera_id))
        {
            CameraInfo& camera_info = m_cameras.Get(viewport.camera_id);
            float aspect                = (float)viewport.width / (float)viewport.height;
            glm::mat4 projection_matrix = glm::perspective(camera_info.camera_fov_y, aspect, camera_info.camera_near,
                                                           camera_info.camera_far);

            EntityInfo& entity    = m_entities_map[camera_info.owner_entity];
            glm::mat4 view_matrix = glm::inverse(entity.current_matrix);
            camera_uniform.projection_view = projection_matrix * view_matrix;
        }
        else
        {
            BRR_LogError(
                "Trying to create Viewport with Camera (ID: {}) that does not exist. Using identity matrix as Camera ProjectionView.",
                uint32_t(viewport.camera_id));
            camera_uniform.projection_view = glm::identity<glm::mat4>();
            viewport.camera_id = CameraID::NULL_ID;
        }

        for (uint32_t frame_idx = 0; frame_idx < FRAME_LAG; frame_idx++)
        {
            viewport.camera_uniform_buffers[frame_idx].Map();
            viewport.camera_uniform_buffers[frame_idx].WriteToBuffer(
                &camera_uniform, sizeof(CameraUniform));
            viewport.camera_uniform_buffers[frame_idx].Unmap();
        }
        BRR_LogInfo("Initialized Viewport Uniform Buffers.");
    }

    void SceneRenderer::SetupEntityUniforms(EntityInfo& entity_info)
    {
        size_t buffer_size = sizeof(glm::mat4);

        for (uint32_t frame_idx = 0; frame_idx < FRAME_LAG; frame_idx++)
        {
            entity_info.uniform_buffers[frame_idx] = DeviceBuffer(buffer_size,
                                                                  UniformBuffer | HostAccessSequencial,
                                                                  MemoryUsage::AUTO);
            entity_info.uniform_buffers[frame_idx].Map();
            entity_info.uniform_buffers[frame_idx].WriteToBuffer(&entity_info.current_matrix);
            entity_info.uniform_buffers[frame_idx].Unmap();

        }

        Shader* shader = RenderStorageGlobals::material_storage.GetShader(m_shader_id);
        assert(shader != nullptr && "Default shader must be initialized when updating SceneRenderer.");

        // Init model descriptor sets
        entity_info.descriptor_layout = shader->GetDescriptorSetLayouts()[1];

        std::vector<DescriptorSetHandle> descriptors_handles = m_render_device->
            DescriptorSet_Allocate(entity_info.descriptor_layout.m_layout_handle,
                                   FRAME_LAG);
        std::ranges::copy(descriptors_handles, entity_info.descriptor_sets.begin());

        for (uint32_t frame_idx = 0; frame_idx < FRAME_LAG; frame_idx++)
        {
            auto setBuilder = DescriptorSetUpdater(entity_info.descriptor_layout);
            setBuilder.BindBuffer(0, entity_info.uniform_buffers[frame_idx].GetHandle(), buffer_size);

            setBuilder.UpdateDescriptorSet(entity_info.descriptor_sets[frame_idx]);
        }
        BRR_LogInfo("Initialized Entity Uniform Buffers.");
    }

    void SceneRenderer::MarkEntityDirty(EntityID entity_id, EntityInfo& entity_info, bool mark_surface, bool mark_uniform)
    {
        if (mark_surface) entity_info.surfaces_dirty = true;
        if (mark_uniform) entity_info.uniform_dirty.fill(true);
        m_dirty_entities.insert(entity_id);
    }

    bool SceneRenderer::CreateNewLight(LightID light_id, Light&& new_light)
    {
        // TODO: Recreate lights buffer when adding more than MAX_LIGHTS
        if (m_scene_lights.Size() == MAX_LIGHTS)
        {
            BRR_LogError("Currently it is not possible to add more than 256 lights for rendering.");
            return false;
        }
        bool result = m_scene_lights.AddObject(light_id, std::move(new_light));

        m_scene_uniform_info.m_light_storage_dirty.fill(true);
        m_scene_uniform_info.m_light_storage_size_changed.fill(true);
        return true;
    }

    void SceneRenderer::ReferenceNewMaterial(MaterialID material_id)
    {
        if (!m_cached_materials.Contains(material_id))
        {
            Material* new_material = RenderStorageGlobals::material_storage.GetMaterial(material_id);
            assert(new_material != nullptr && "Referenced Material must be a valid material.");
            m_cached_materials.AddObject(material_id, {new_material->descriptor_sets, 1});
        }
        else
        {
            MaterialRenderData& new_material_render_data = m_cached_materials.Get(material_id);
            new_material_render_data.reference_count++;
        }
    }

    void SceneRenderer::DereferenceMaterial(MaterialID material_id)
    {
        // Decrease material reference count, and erase it from cached materials if no more references.
        if (m_cached_materials.Contains(material_id))
        {
            MaterialRenderData& old_material_render_data = m_cached_materials.Get(material_id);
            if (old_material_render_data.reference_count > 0)
            {
                old_material_render_data.reference_count--;
            }
            if (old_material_render_data.reference_count == 0)
            {
                m_cached_materials.RemoveObject(material_id);
            }
        }
    }
}
