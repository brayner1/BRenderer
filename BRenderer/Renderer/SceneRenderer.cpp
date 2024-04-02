#include "SceneRenderer.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Scene/Components/Transform3DComponent.h>
#include <Core/LogSystem.h>

#define MAX_LIGHTS 256

struct CameraUniform
{
    glm::mat4 projection_view{ 1.f };
};

struct Transform3DUniform
{
    glm::mat4 model_matrix;
};

namespace brr::render
{
    SceneRenderer::SceneRenderer(Scene* scene)
    : m_scene(scene),
      m_render_device(render::VKRD::GetSingleton())
    {
        assert(m_render_device && "Can't create SceneRenderer without VulkanRenderDevice.");
        BRR_LogInfo("Creating SceneRenderer");

        {
            render::ShaderBuilder shader_builder;
            shader_builder
            .SetVertexShaderFile ("vert.spv")
            .SetFragmentShaderFile ("frag.spv")
            .AddVertexInputBindingDescription (0, sizeof(Vertex3))
            .AddVertexAttributeDescription(0, 0, render::DataFormat::R32G32B32_Float, offsetof(Vertex3, pos))
            .AddVertexAttributeDescription(0, 1, render::DataFormat::R32_Float, offsetof(Vertex3, u))
            .AddVertexAttributeDescription(0, 2, render::DataFormat::R32G32B32_Float, offsetof(Vertex3, normal))
            .AddVertexAttributeDescription(0, 3, render::DataFormat::R32_Float, offsetof(Vertex3, v))
            .AddVertexAttributeDescription(0, 4, render::DataFormat::R32G32B32_Float, offsetof(Vertex3, tangent))
            .AddSet()
            .AddSetBinding(render::DescriptorType::UniformBuffer, render::ShaderStageFlag::VertexShader)
            .AddSetBinding(render::DescriptorType::StorageBuffer, render::ShaderStageFlag::FragmentShader)
            .AddSet()
            .AddSetBinding(render::DescriptorType::UniformBuffer, render::ShaderStageFlag::VertexShader)
            .AddSet()
            .AddSetBinding(render::DescriptorType::CombinedImageSampler, render::ShaderStageFlag::FragmentShader);

            m_shader = shader_builder.BuildShader();
            m_graphics_pipeline = m_render_device->Create_GraphicsPipeline(m_shader, {render::DataFormat::R8G8B8A8_SRGB}, render::DataFormat::D32_Float);
            m_shader.DestroyShaderModules();
        }
        //CreatePointLight(glm::vec3(0.0, 6.0, 0.0), glm::vec3(1.0, 0.8, 0.8), 2.0);

        SetupCameraUniforms();

        m_image = std::make_unique<vis::Image>("Resources/UV_Grid.png");

        m_texture_2d_handle = m_render_device->Create_Texture2D(m_image->Width(), m_image->Height(),
                                                                render::ImageUsage::TransferDstImage | render::ImageUsage::SampledImage,
                                                                render::DataFormat::R8G8B8A8_SRGB);
        if (m_texture_2d_handle != null_handle)
        {
            m_render_device->UpdateTexture2DData(m_texture_2d_handle, m_image->Data(), m_image->DataSize(), {0, 0}, {m_image->Width(), m_image->Height()});
        }
        SetupMaterialUniforms();
    }

    SceneRenderer::~SceneRenderer()
    {
        for (RenderData& render_data : m_render_data)
        {
            DestroyBuffers(render_data);
        }
        m_render_device->DestroyTexture2D(m_texture_2d_handle);
        m_render_device->DestroyGraphicsPipeline(m_graphics_pipeline);
    }

    SurfaceId SceneRenderer::CreateNewSurface(Mesh3DComponent::SurfaceData& surface,
                                              const Entity& owner_entity)
    {
        NodeComponent& owner_node = owner_entity.GetComponent<NodeComponent>();
        const ContiguousPool<RenderData>::ObjectId render_data_id = m_render_data.AddNewObject({&owner_node});

        SurfaceId surface_id = static_cast<SurfaceId>(render_data_id);
        BRR_LogInfo("Adding new RenderData for Surface with ID: {}", static_cast<uint64_t>(surface_id));
        
        RenderData& render_data = m_render_data.Get(render_data_id);

        auto& vertices = const_cast<std::vector<Vertex3>&> (surface.GetVertices());
        auto& indices = const_cast<std::vector<uint32_t>&> (surface.GetIndices());

        CreateVertexBuffer(vertices, render_data);
        CreateIndexBuffer(indices, render_data);

        return surface_id;
    }

    void SceneRenderer::RemoveSurface(SurfaceId surface_id)
    {
        ContiguousPool<RenderData>::ObjectId object_id = static_cast<ContiguousPool<RenderData>::ObjectId>(surface_id);
        RenderData& render_data = m_render_data.Get(object_id);

        DestroyBuffers(render_data);

        m_render_data.RemoveObject(object_id);
    }

    LightId SceneRenderer::CreatePointLight(const glm::vec3& position, const glm::vec3& color, float intensity)
    {
        Light point_light;
        point_light.light_type = 0;
        point_light.light_position = position;
        point_light.light_intensity = intensity;
        point_light.light_color = color;
        LightId light_id = CreateNewLight(std::move(point_light));

        BRR_LogInfo("Created new PointLight. LightId: {}", static_cast<uint32_t>(light_id));
        return light_id;
    }

    void SceneRenderer::UpdatePointLight(LightId light_id, const glm::vec3& position, const glm::vec3& color,
        float intensity)
    {
        Light& point_light = m_scene_lights.Get(static_cast<ContiguousPool<Light>::ObjectId>(light_id));
        point_light.light_position = position;
        point_light.light_color = color;
        point_light.light_intensity = intensity;

        m_camera_uniform_info.m_light_storage_dirty.fill(true);
    }

    LightId SceneRenderer::CreateDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
    {
        Light directional_light;
        directional_light.light_type = 1;
        directional_light.light_intensity = intensity;
        directional_light.light_direction = direction;
        directional_light.light_color = color;
        LightId light_id = CreateNewLight(std::move(directional_light));

        BRR_LogInfo("Created new DirectionalLight. LightId: {}", static_cast<uint32_t>(light_id));
        return light_id;
    }

    void SceneRenderer::UpdateDirectionalLight(LightId light_id, const glm::vec3& direction, const glm::vec3& color,
        float intensity)
    {
        Light& directional_light = m_scene_lights.Get(static_cast<ContiguousPool<Light>::ObjectId>(light_id));
        directional_light.light_intensity = intensity;
        directional_light.light_direction = direction;
        directional_light.light_color = color;

        m_camera_uniform_info.m_light_storage_dirty.fill(true);
    }

    LightId SceneRenderer::CreateSpotLight(const glm::vec3& position, float cutoff_angle, const glm::vec3& direction,
                                           float intensity, const glm::vec3& color)
    {
        Light spot_light;
        spot_light.light_type = 2;
        spot_light.light_position = position;
        spot_light.light_intensity = intensity;
        spot_light.light_direction = direction;
        spot_light.light_color = color;
        spot_light.light_cutoff = std::cos(cutoff_angle);
        LightId light_id = CreateNewLight(std::move(spot_light));

        BRR_LogInfo("Created new SpotLight. LightId: {}", static_cast<uint32_t>(light_id));
        return light_id;
    }

    void SceneRenderer::UpdateSpotLight(LightId light_id, const glm::vec3& position, float cutoff_angle,
        const glm::vec3& direction, float intensity, const glm::vec3& color)
    {
        Light& spot_light = m_scene_lights.Get(static_cast<ContiguousPool<Light>::ObjectId>(light_id));
        spot_light.light_position = position;
        spot_light.light_intensity = intensity;
        spot_light.light_direction = direction;
        spot_light.light_color = color;
        spot_light.light_cutoff = std::cos(cutoff_angle);

        m_camera_uniform_info.m_light_storage_dirty.fill(true);
    }

    LightId SceneRenderer::CreateAmbientLight(const glm::vec3& color, float intensity)
    {
        Light ambient_light;
        ambient_light.light_type = 3;
        ambient_light.light_intensity = intensity;
        ambient_light.light_color = color;
        LightId light_id = CreateNewLight(std::move(ambient_light));

        BRR_LogInfo("Created new AmbientLight. LightId: {}", static_cast<uint32_t>(light_id));
        return light_id;
    }

    void SceneRenderer::UpdateAmbientLight(LightId light_id, const glm::vec3& color, float intensity)
    {
        Light& ambient_light = m_scene_lights.Get(static_cast<ContiguousPool<Light>::ObjectId>(light_id));
        ambient_light.light_intensity = intensity;
        ambient_light.light_color = color;

        m_camera_uniform_info.m_light_storage_dirty.fill(true);
    }

    void SceneRenderer::RemoveLight(LightId light_id)
    {
        m_scene_lights.RemoveObject(static_cast<ContiguousPool<Light>::ObjectId>(light_id));

        m_camera_uniform_info.m_light_storage_dirty.fill(true);
        m_camera_uniform_info.m_light_storage_size_changed.fill(true);

        BRR_LogInfo("Removed Light. LightId: {}", static_cast<uint32_t>(light_id));
    }

    ViewportId SceneRenderer::CreateViewport(glm::uvec2 viewport_size)
    {
        Viewport viewport {viewport_size.x, viewport_size.y};
        for (uint32_t idx = 0; idx < render::FRAME_LAG; idx++)
        {
            viewport.color_attachment[idx] = m_render_device->Create_Texture2D(viewport.width, viewport.heigth,
                                                                   render::ImageUsage::ColorAttachmentImage | render::ImageUsage::TransferSrcImage,
                                                                   render::DataFormat::R8G8B8A8_SRGB);
            viewport.depth_attachment[idx] = m_render_device->Create_Texture2D(viewport.width, viewport.heigth,
                                                                   render::ImageUsage::DepthStencilAttachmentImage,
                                                                   render::DataFormat::D32_Float);

        }
        auto viewport_obj_id = m_viewports.AddNewObject(std::move(viewport));
        BRR_LogInfo("Created Viewport. ViewportId: {}", static_cast<uint32_t>(viewport_obj_id));
        return static_cast<ViewportId>(viewport_obj_id);
    }

    void SceneRenderer::ResizeViewport(ViewportId viewport_id,
                                       glm::uvec2 new_size)
    {
        Viewport& viewport = m_viewports.Get(static_cast<ContiguousPool<Viewport>::ObjectId>(viewport_id));
        viewport.width = new_size.x;
        viewport.heigth = new_size.y;
        for (uint32_t idx = 0; idx < render::FRAME_LAG; idx++)
        {
            m_render_device->DestroyTexture2D(viewport.color_attachment[idx]);
            viewport.color_attachment[idx] = m_render_device->Create_Texture2D(viewport.width, viewport.heigth,
                                                                   render::ImageUsage::ColorAttachmentImage | render::ImageUsage::TransferSrcImage,
                                                                   render::DataFormat::R8G8B8A8_SRGB);

            m_render_device->DestroyTexture2D(viewport.depth_attachment[idx]);
            viewport.depth_attachment[idx] = m_render_device->Create_Texture2D(viewport.width, viewport.heigth,
                                                                   render::ImageUsage::DepthStencilAttachmentImage,
                                                                   render::DataFormat::D32_Float);

        }
        BRR_LogInfo("Resized Viewport. ViewportId: {}", static_cast<uint32_t>(viewport_id));
    }

    void SceneRenderer::RemoveViewport(ViewportId viewport_id)
    {
        Viewport& viewport = m_viewports.Get(static_cast<ContiguousPool<Viewport>::ObjectId>(viewport_id));
        for (uint32_t idx = 0; idx < render::FRAME_LAG; idx++)
        {
            m_render_device->DestroyTexture2D(viewport.color_attachment[idx]);
            m_render_device->DestroyTexture2D(viewport.depth_attachment[idx]);
        }

        m_viewports.RemoveObject(static_cast<ContiguousPool<Viewport>::ObjectId>(viewport_id));
        BRR_LogInfo("Destroyed Viewport. ViewportId: {}", static_cast<uint32_t>(viewport_id));
    }

    void SceneRenderer::UpdateDirtyInstances()
    {
        assert(m_render_device && "VulkanRenderDevice must be initialized on construction.");
        m_current_buffer = m_render_device->GetCurrentFrameBufferIndex();

        for (RenderData& render_data : m_render_data)
        {
            NodeComponent* node = render_data.m_owner_node;
            Transform3DComponent& transform = node->GetEntity().GetComponent<Transform3DComponent>();

            // update transformation uniform
            if (transform.Dirty() != Transform3DComponent::NOT_DIRTY || render_data.m_uniform_dirty[m_current_buffer])
            {
                if (!render_data.m_descriptor_sets[0])
                {
                    InitRenderDataUniforms(render_data);

                    BRR_LogInfo("Model Descriptor Sets created.");
                }

                if (transform.Dirty() != Transform3DComponent::NOT_DIRTY)
                {
                    render_data.m_uniform_dirty.fill(true);
                }

                Transform3DUniform uniform{};
                uniform.model_matrix = transform.GetGlobalTransform();

                render_data.m_uniform_buffers[m_current_buffer].Map();
                render_data.m_uniform_buffers[m_current_buffer].WriteToBuffer(&uniform, sizeof(uniform));
                render_data.m_uniform_buffers[m_current_buffer].Unmap();

                render_data.m_uniform_dirty[m_current_buffer] = false;
            }
        }

        CameraUniform ubo;
        ubo.projection_view = m_scene->GetMainCamera()->GetProjectionMatrix() * m_scene->GetMainCamera()->GetViewMatrix();

        m_camera_uniform_info.m_camera_uniforms[m_current_buffer].Map();
        m_camera_uniform_info.m_camera_uniforms[m_current_buffer].WriteToBuffer(&ubo, sizeof(ubo));
        m_camera_uniform_info.m_camera_uniforms[m_current_buffer].Unmap();

        if (m_camera_uniform_info.m_light_storage_dirty[m_current_buffer])
        {
            m_camera_uniform_info.m_light_storage_dirty[m_current_buffer] = false;

            m_camera_uniform_info.m_lights_buffers[m_current_buffer].Map();
            m_camera_uniform_info.m_lights_buffers[m_current_buffer].WriteToBuffer(m_scene_lights.Data(), m_scene_lights.Size() * sizeof(Light));
            m_camera_uniform_info.m_lights_buffers[m_current_buffer].Unmap();

            if (m_camera_uniform_info.m_light_storage_size_changed[m_current_buffer])
            {
                m_camera_uniform_info.m_light_storage_size_changed[m_current_buffer] = false;

                render::DescriptorLayout layout = m_shader.GetDescriptorSetLayouts()[0];
                auto setBuilder = render::DescriptorSetUpdater(layout);
                setBuilder.BindBuffer(1, m_camera_uniform_info.m_lights_buffers[m_current_buffer].GetHandle(), m_scene_lights.Size() * sizeof(Light));

                setBuilder.UpdateDescriptorSet(m_camera_uniform_info.m_descriptor_sets[m_current_buffer]);
            }
        }
    }

    void SceneRenderer::Render3D(ViewportId viewport_id, render::Texture2DHandle render_target)
    {
        Viewport& viewport = m_viewports.Get(static_cast<ContiguousPool<Viewport>::ObjectId>(viewport_id));

        m_render_device->RenderTarget_BeginRendering(viewport.color_attachment[m_current_buffer], viewport.depth_attachment[m_current_buffer]);

        m_render_device->Bind_GraphicsPipeline(m_graphics_pipeline);

        m_render_device->Bind_DescriptorSet(m_graphics_pipeline, m_camera_uniform_info.m_descriptor_sets[m_current_buffer], 0);
        for (RenderData& render_data : m_render_data)
        {
            m_render_device->Bind_DescriptorSet(m_graphics_pipeline, render_data.m_descriptor_sets[m_current_buffer], 1);
            m_render_device->Bind_DescriptorSet(m_graphics_pipeline, m_material_descriptor_sets[m_current_buffer], 2);

            assert(render_data.m_vertex_buffer_handle.IsValid() && "Vertex buffer must be valid to bind to a command buffer.");
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

    void SceneRenderer::SetupCameraUniforms()
    {
        for (size_t idx = 0; idx < render::FRAME_LAG; idx++)
        {
            m_camera_uniform_info.m_camera_uniforms[idx] =
                render::DeviceBuffer(sizeof(CameraUniform),
                                     render::BufferUsage::UniformBuffer | render::BufferUsage::HostAccessSequencial,
                                     render::MemoryUsage::AUTO);
            m_camera_uniform_info.m_lights_buffers[idx] =
                render::DeviceBuffer(sizeof(Light) * MAX_LIGHTS,
                                     render::BufferUsage::StorageBuffer | render::BufferUsage::HostAccessSequencial,
                                     render::MemoryUsage::AUTO);
        }

        render::DescriptorLayout layout = m_shader.GetDescriptorSetLayouts()[0];

        std::vector<render::DescriptorSetHandle> descriptors_handles = m_render_device->AllocateDescriptorSet(layout.m_layout_handle, render::FRAME_LAG);
        std::ranges::copy(descriptors_handles, m_camera_uniform_info.m_descriptor_sets.begin());

        for (uint32_t set_idx = 0; set_idx < render::FRAME_LAG; set_idx++)
        {
            auto setBuilder = render::DescriptorSetUpdater(layout);
            setBuilder.BindBuffer(0, m_camera_uniform_info.m_camera_uniforms[set_idx].GetHandle(), sizeof(CameraUniform));
            setBuilder.BindBuffer(1, m_camera_uniform_info.m_lights_buffers[set_idx].GetHandle(), sizeof(Light));

            setBuilder.UpdateDescriptorSet(m_camera_uniform_info.m_descriptor_sets[set_idx]);
        }
    }

    void SceneRenderer::SetupMaterialUniforms()
    {
        // Init material descriptor set
        {
            m_material_descriptor_layout = m_shader.GetDescriptorSetLayouts()[2];

            std::vector<render::DescriptorSetHandle> descriptors_handles = m_render_device->AllocateDescriptorSet(
                m_material_descriptor_layout.m_layout_handle, render::FRAME_LAG);
            std::ranges::copy(descriptors_handles, m_material_descriptor_sets.begin());

            for (uint32_t set_idx = 0; set_idx < render::FRAME_LAG; set_idx++)
            {
                auto setBuilder = render::DescriptorSetUpdater(m_material_descriptor_layout);

                setBuilder.BindImage(0, m_texture_2d_handle);

                setBuilder.UpdateDescriptorSet(m_material_descriptor_sets[set_idx]);
            }
        }
    }

    void SceneRenderer::InitRenderDataUniforms(RenderData& render_data)
    {
        size_t buffer_size = sizeof(Transform3DUniform);

        BRR_LogInfo("Creating Uniform Buffers");
        for (uint32_t i = 0; i < render::FRAME_LAG; i++)
        {
            render_data.m_uniform_buffers[i] = render::DeviceBuffer(buffer_size,
                                                                    render::BufferUsage::UniformBuffer | render::BufferUsage::HostAccessSequencial,
                                                                    render::MemoryUsage::AUTO);
        }

        // Init model descriptor sets
        render_data.m_model_descriptor_layout = m_shader.GetDescriptorSetLayouts()[1];

        std::vector<render::DescriptorSetHandle> descriptors_handles = m_render_device->
            AllocateDescriptorSet(render_data.m_model_descriptor_layout.m_layout_handle,
                                  render::FRAME_LAG);
        std::ranges::copy(descriptors_handles, render_data.m_descriptor_sets.begin());

        for (uint32_t set_idx = 0; set_idx < render::FRAME_LAG; set_idx++)
        {
            auto setBuilder = render::DescriptorSetUpdater(render_data.m_model_descriptor_layout);
            setBuilder.BindBuffer(0, render_data.m_uniform_buffers[set_idx].GetHandle(), buffer_size);

            setBuilder.UpdateDescriptorSet(render_data.m_descriptor_sets[set_idx]);
        }
        BRR_LogInfo("Uniform Buffers created.");
    }

    LightId SceneRenderer::CreateNewLight(Light&& new_light)
    {
        // TODO: Recreate lights buffer when adding more than MAX_LIGHTS
        if (m_scene_lights.Size() == MAX_LIGHTS)
        {
            BRR_LogError("Currently it is not possible to add more than 256 lights for rendering.");
            return LightId::NULL_ID;
        }
        LightId light_id = static_cast<LightId>(m_scene_lights.AddNewObject(std::move(new_light)));

        m_camera_uniform_info.m_light_storage_dirty.fill(true);
        m_camera_uniform_info.m_light_storage_size_changed.fill(true);
        return light_id;
    }

    void SceneRenderer::CreateVertexBuffer(std::vector<Vertex3>& vertex_buffer, RenderData& render_data)
    {
        assert(!vertex_buffer.empty() && "Vertices data can't be empty.");

        const size_t buffer_size = sizeof(Vertex3) * vertex_buffer.size();

        BRR_LogInfo("Creating Vertex Buffer.");

        render::VulkanRenderDevice::VertexFormatFlags vertex_format = render::VulkanRenderDevice::VertexFormatFlags::COLOR;

        render_data.m_vertex_buffer_handle = m_render_device->CreateVertexBuffer(buffer_size, vertex_format);
        m_render_device->UpdateVertexBufferData(render_data.m_vertex_buffer_handle, vertex_buffer.data(), buffer_size, 0);

        render_data.num_vertices = vertex_buffer.size();
    }

    void SceneRenderer::CreateIndexBuffer(std::vector<uint32_t>& index_buffer, RenderData& render_data)
    {
        if (index_buffer.empty())
            return;

        size_t buffer_size = sizeof(uint32_t) * index_buffer.size();

        BRR_LogInfo("Creating Index Buffer.");

        render::VulkanRenderDevice::IndexType index_type = render::VulkanRenderDevice::IndexType::UINT32;

        render_data.m_index_buffer_handle = m_render_device->CreateIndexBuffer(buffer_size, index_type, index_buffer.data());

        render_data.num_indices = index_buffer.size();
    }

    void SceneRenderer::DestroyBuffers(RenderData& render_data)
    {
        m_render_device->DestroyVertexBuffer(render_data.m_vertex_buffer_handle);
        m_render_device->DestroyIndexBuffer(render_data.m_index_buffer_handle);
    }
}
