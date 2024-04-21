#include "SceneRenderer.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Core/LogSystem.h>

#include "Core/App.h"
#include "Core/App.h"
#include "Core/App.h"
#include "Core/App.h"
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
            // Sets should be organized from less frequently updated to more frequently updated.
            ShaderBuilder shader_builder;
            shader_builder
                .SetVertexShaderFile("vert.spv")
                .SetFragmentShaderFile("frag.spv")
                .AddVertexInputBindingDescription(0, sizeof(Vertex3))
                .AddVertexAttributeDescription(0, 0, DataFormat::R32G32B32_Float, offsetof(Vertex3, pos))
                .AddVertexAttributeDescription(0, 1, DataFormat::R32_Float, offsetof(Vertex3, u))
                .AddVertexAttributeDescription(0, 2, DataFormat::R32G32B32_Float, offsetof(Vertex3, normal))
                .AddVertexAttributeDescription(0, 3, DataFormat::R32_Float, offsetof(Vertex3, v))
                .AddVertexAttributeDescription(0, 4, DataFormat::R32G32B32_Float, offsetof(Vertex3, tangent))
                .AddSet() // Set 0 -> Binding 0: Lights array
                .AddSetBinding(DescriptorType::StorageBuffer, FragmentShader)
                .AddSet() // Set 1 -> Binding 0: Camera view and projection
                .AddSetBinding(DescriptorType::UniformBuffer, VertexShader)
                .AddSet() // Set 2 -> Binding 0: Material transform.
                .AddSetBinding(DescriptorType::CombinedImageSampler, FragmentShader)
                .AddSet() // Set 3 -> Binding 0: Model Uniform
                .AddSetBinding(DescriptorType::UniformBuffer, VertexShader);

            m_shader            = shader_builder.BuildShader();
            m_graphics_pipeline = m_render_device->Create_GraphicsPipeline(m_shader, {DataFormat::R8G8B8A8_SRGB},
                                                                           DataFormat::D32_Float);
            m_shader.DestroyShaderModules();
        }

        SetupSceneUniforms();

        m_image = std::make_unique<vis::Image>("Resources/UV_Grid.png");

        m_texture_2d_handle = m_render_device->Create_Texture2D(m_image->Width(), m_image->Height(),
                                                                ImageUsage::TransferDstImage |
                                                                ImageUsage::SampledImage,
                                                                DataFormat::R8G8B8A8_SRGB);
        if (m_texture_2d_handle != null_handle)
        {
            m_render_device->UpdateTexture2DData(m_texture_2d_handle, m_image->Data(), m_image->DataSize(), {0, 0},
                                                 {m_image->Width(), m_image->Height()});
        }
        SetupMaterialUniforms();
    }

    SceneRenderer::~SceneRenderer()
    {
        for (SurfaceRenderData& render_data : m_render_data)
        {
            DestroySurfaceBuffers(render_data);
        }
        m_render_device->DestroyTexture2D(m_texture_2d_handle);
        m_render_device->DestroyGraphicsPipeline(m_graphics_pipeline);
    }

    void SceneRenderer::CreateCamera(CameraId camera_id,
                                     EntityId owner_entity,
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

    void SceneRenderer::DestroyCamera(CameraId camera_id)
    {
        if (!m_cameras.Contains(camera_id))
        {
            BRR_LogError("Can't destroy SceneRenderer Camera (ID: {}) because this Camera doesn't exist.",
                         static_cast<uint32_t>(camera_id));
            return;
        }

        m_cameras.RemoveObject(camera_id);
        // TODO: Need to check and update viewports that use this camera!
    }

    void SceneRenderer::UpdateCameraProjection(CameraId camera_id,
                                               float camera_fovy,
                                               float camera_near,
                                               float camera_far)
    {
        CameraInfo& camera_info  = m_cameras.Get(camera_id);
        camera_info.camera_fov_y = camera_fovy;
        camera_info.camera_near  = camera_near;
        camera_info.camera_far   = camera_far;

        // TODO: Need to check and update viewports that use this camera!
    }

    void SceneRenderer::CreateEntity(EntityId entity_id,
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

    void SceneRenderer::DestroyEntity(EntityId entity_id)
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
    }

    void SceneRenderer::UpdateEntityTransform(EntityId entity_id,
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
        entity_it->second.uniform_dirty.fill(true);
    }

    void SceneRenderer::CreateSurface(SurfaceId surface_id,
                                         EntityId owner_entity,
                                         void* vertex_buffer_data,
                                         size_t vertex_buffer_size,
                                         void* index_buffer_data,
                                         size_t index_buffer_size)
    {
        if (!m_render_data.AddObject(surface_id, {owner_entity}))
        {
            BRR_LogError("Can't create Surface (ID: {}). A surface with this ID already exists.",
                         static_cast<uint32_t>(surface_id));
            return;
        }

        BRR_LogDebug("Adding new SurfaceRenderData (ID: {}).", static_cast<uint32_t>(surface_id));

        SurfaceRenderData& render_data = m_render_data.Get(surface_id);
        render_data.m_my_surface_id = surface_id;
        CreateVertexBuffer(vertex_buffer_data, vertex_buffer_size, render_data);
        CreateIndexBuffer(index_buffer_data, index_buffer_size, render_data);
    }

    void SceneRenderer::DestroySurface(SurfaceId surface_id)
    {
        if (!m_render_data.Contains(surface_id))
        {
            BRR_LogError("Can't destroy Surface (ID: {}). This surface doesn't exist.", static_cast<uint32_t>(surface_id));
            return;
        }
        SurfaceRenderData& render_data = m_render_data.Get(surface_id);
        DestroySurfaceBuffers(render_data);

        m_render_data.RemoveObject(surface_id);
    }

    void SceneRenderer::UpdateSurfaceVertexBuffer(SurfaceId surface_id,
                                                  void* vertex_buffer_data,
                                                  size_t vertex_buffer_size)
    {
    }

    void SceneRenderer::UpdateSurfaceIndexBuffer(SurfaceId surface_id,
                                                 void* index_buffer_data,
                                                 size_t index_buffer_size)
    {
    }

    void SceneRenderer::CreatePointLight(LightId light_id,
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

        BRR_LogInfo("Created new PointLight. LightId: {}", static_cast<uint32_t>(light_id));
    }

    void SceneRenderer::UpdatePointLight(LightId light_id,
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

    void SceneRenderer::CreateDirectionalLight(LightId light_id,
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

        BRR_LogInfo("Created new DirectionalLight. LightId: {}", static_cast<uint32_t>(light_id));
    }

    void SceneRenderer::UpdateDirectionalLight(LightId light_id,
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

    void SceneRenderer::CreateSpotLight(LightId light_id,
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

        BRR_LogInfo("Created new SpotLight. LightId: {}", static_cast<uint32_t>(light_id));
    }

    void SceneRenderer::UpdateSpotLight(LightId light_id,
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

    void SceneRenderer::CreateAmbientLight(LightId light_id,
                                           const glm::vec3& color,
                                           float intensity)
    {
        Light ambient_light;
        ambient_light.light_type      = 3;
        ambient_light.light_intensity = intensity;
        ambient_light.light_color     = color;
        CreateNewLight(light_id, std::move(ambient_light));

        BRR_LogInfo("Created new AmbientLight. LightId: {}", static_cast<uint32_t>(light_id));
    }

    void SceneRenderer::UpdateAmbientLight(LightId light_id,
                                           const glm::vec3& color,
                                           float intensity)
    {
        Light& ambient_light          = m_scene_lights.Get(light_id);
        ambient_light.light_intensity = intensity;
        ambient_light.light_color     = color;

        m_scene_uniform_info.m_light_storage_dirty.fill(true);
    }

    void SceneRenderer::DestroyLight(LightId light_id)
    {
        m_scene_lights.RemoveObject(light_id);

        m_scene_uniform_info.m_light_storage_dirty.fill(true);
        m_scene_uniform_info.m_light_storage_size_changed.fill(true);

        BRR_LogInfo("Removed Light. LightId: {}", static_cast<uint32_t>(light_id));
    }

    ViewportId SceneRenderer::CreateViewport(glm::uvec2 viewport_size, CameraId camera_id)
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
        ViewportId new_viewport_id = static_cast<ViewportId>(s_viewport_id_owner.GetNewId());
        m_viewports.AddObject(new_viewport_id, std::move(viewport));

        Viewport& new_viewport = m_viewports.Get(new_viewport_id);
        SetupViewportUniforms(new_viewport);

        BRR_LogInfo("Created Viewport. Viewport ID: {}. Viewport Size: (width: {}, height: {})", static_cast<uint32_t>(new_viewport_id), viewport_size.x, viewport_size.y);
        return new_viewport_id;
    }

    void SceneRenderer::ResizeViewport(ViewportId viewport_id,
                                       glm::uvec2 new_size)
    {
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

    void SceneRenderer::DestroyViewport(ViewportId viewport_id)
    {
        Viewport& viewport = m_viewports.Get(viewport_id);
        for (uint32_t idx = 0; idx < FRAME_LAG; idx++)
        {
            m_render_device->DestroyTexture2D(viewport.color_attachment[idx]);
            m_render_device->DestroyTexture2D(viewport.depth_attachment[idx]);
        }

        m_viewports.RemoveObject(viewport_id);
        BRR_LogInfo("Destroyed Viewport. Viewport ID: {}", static_cast<uint32_t>(viewport_id));
    }

    void SceneRenderer::UpdateDirtyInstances()
    {
        assert(m_render_device && "VulkanRenderDevice must be initialized on construction.");
        m_current_buffer = m_render_device->GetCurrentFrameBufferIndex();

        std::set<EntityId> updated_entities;
        for (auto& entity_iter : m_entities_map)
        {
            EntityInfo& entity = entity_iter.second;
            if (entity.uniform_dirty[m_current_buffer])
            {
                entity.uniform_buffers[m_current_buffer].Map();
                entity.uniform_buffers[m_current_buffer].WriteToBuffer(
                    &entity.current_matrix, sizeof(glm::mat4));
                entity.uniform_buffers[m_current_buffer].Unmap();

                entity.uniform_dirty[m_current_buffer] = false;
                updated_entities.emplace(entity_iter.first);
            }
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

                DescriptorLayout layout = m_shader.GetDescriptorSetLayouts()[0];
                auto setBuilder                 = DescriptorSetUpdater(layout);
                setBuilder.BindBuffer(0, m_scene_uniform_info.m_lights_buffers[m_current_buffer].GetHandle(),
                                      m_scene_lights.Size() * sizeof(Light));

                setBuilder.UpdateDescriptorSet(m_scene_uniform_info.m_descriptor_sets[m_current_buffer]);
            }
        }
    }

    void SceneRenderer::Render3D(ViewportId viewport_id,
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

        // Material uniform
        m_render_device->Bind_DescriptorSet(m_graphics_pipeline, m_material_descriptor_sets[m_current_buffer], 2);
        // TODO: create a proper material system

        for (SurfaceRenderData& render_data : m_render_data)
        {
            // Entity uniform (model matrix)
            auto entity_iter = m_entities_map.find(render_data.m_owner_node);
            if (entity_iter == m_entities_map.end())
            {
                BRR_LogError(
                    "Surface (ID: {}) is owned by non-existing Entity (ID: {}).\nSkipping rendering of this Surface.",
                    uint32_t(render_data.m_my_surface_id), uint32_t(render_data.m_owner_node));
                continue;
            }
            EntityInfo& entity_info = entity_iter->second;
            m_render_device->Bind_DescriptorSet(m_graphics_pipeline, entity_info.descriptor_sets[m_current_buffer], 3);

            assert(
                render_data.m_vertex_buffer_handle.IsValid() && "Vertex buffer must be valid to bind to a command buffer.");
            m_render_device->BindVertexBuffer(render_data.m_vertex_buffer_handle);

            if (render_data.m_index_buffer_handle.IsValid())
            {
                m_render_device->BindIndexBuffer(render_data.m_index_buffer_handle);
                m_render_device->DrawIndexed(render_data.num_indices, 1, 0, 0, 0);
            }
            else
            {
                m_render_device->Draw(render_data.num_vertices, 1, 0, 0);
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

        DescriptorLayout layout = m_shader.GetDescriptorSetLayouts()[0];

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

        // Init viewport camera descriptor sets
        DescriptorLayout descriptor_layout = m_shader.GetDescriptorSetLayouts()[1];

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

        CameraInfo& camera_info = m_cameras.Get(viewport.camera_id);
        float aspect                = (float)viewport.width / (float)viewport.height;
        glm::mat4 projection_matrix = glm::perspective(camera_info.camera_fov_y, aspect, camera_info.camera_near,
                                                       camera_info.camera_far);

        EntityInfo& entity    = m_entities_map[camera_info.owner_entity];
        glm::mat4 view_matrix = glm::inverse(entity.current_matrix);

        CameraUniform camera_uniform;
        camera_uniform.projection_view = projection_matrix * view_matrix;
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

        // Init model descriptor sets
        entity_info.descriptor_layout = m_shader.GetDescriptorSetLayouts()[1];

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

    void SceneRenderer::SetupMaterialUniforms()
    {
        // Init material descriptor set
        {
            m_material_descriptor_layout = m_shader.GetDescriptorSetLayouts()[2];

            std::vector<DescriptorSetHandle> descriptors_handles = m_render_device->DescriptorSet_Allocate(
                m_material_descriptor_layout.m_layout_handle, FRAME_LAG);
            std::ranges::copy(descriptors_handles, m_material_descriptor_sets.begin());

            for (uint32_t frame_idx = 0; frame_idx < FRAME_LAG; frame_idx++)
            {
                auto setBuilder = DescriptorSetUpdater(m_material_descriptor_layout);

                setBuilder.BindImage(0, m_texture_2d_handle);

                setBuilder.UpdateDescriptorSet(m_material_descriptor_sets[frame_idx]);
            }
        }
    }

    bool SceneRenderer::CreateNewLight(LightId light_id, Light&& new_light)
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

    void SceneRenderer::CreateVertexBuffer(void* vertex_buffer_data,
                                           size_t vertex_buffer_size,
                                           SurfaceRenderData& render_data)
    {
        if (!vertex_buffer_data || vertex_buffer_size == 0)
        {
            BRR_LogError("Vertex Buffer data can't be null or empty.");
            return;
        }

        BRR_LogInfo("Creating Vertex Buffer.");

        VulkanRenderDevice::VertexFormatFlags vertex_format = VulkanRenderDevice::VertexFormatFlags::COLOR;
        Vertex3* vertex3_data = (Vertex3*)vertex_buffer_data;
        render_data.m_vertex_buffer_handle = m_render_device->CreateVertexBuffer(
            vertex_buffer_size, vertex_format, vertex_buffer_data);

        render_data.num_vertices = vertex_buffer_size / sizeof(Vertex3);
        // TODO: Calculate vertex number based on vertex buffer format
    }

    void SceneRenderer::CreateIndexBuffer(void* index_buffer_data,
                                          size_t index_buffer_size,
                                          SurfaceRenderData& render_data)
    {
        if (!index_buffer_data || index_buffer_size == 0)
            return;

        BRR_LogInfo("Creating Index Buffer.");

        VulkanRenderDevice::IndexType index_type = VulkanRenderDevice::IndexType::UINT32;

        render_data.m_index_buffer_handle = m_render_device->CreateIndexBuffer(
            index_buffer_size, index_type, index_buffer_data);

        render_data.num_indices = index_buffer_size / sizeof(uint32_t);
    }

    void SceneRenderer::DestroySurfaceBuffers(SurfaceRenderData& render_data)
    {
        m_render_device->DestroyVertexBuffer(render_data.m_vertex_buffer_handle);
        m_render_device->DestroyIndexBuffer(render_data.m_index_buffer_handle);
    }
}
