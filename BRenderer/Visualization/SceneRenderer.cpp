#include "SceneRenderer.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Scene/Components/Transform3DComponent.h>
#include <Core/LogSystem.h>

namespace brr::vis
{
    SceneRenderer::SceneRenderer(Scene* scene)
    : m_scene(scene),
      m_render_device(render::VKRD::GetSingleton())
    {
		assert(m_render_device && "Can't create SceneRenderer without VulkanRenderDevice.");
		BRR_LogInfo("Creating SceneRenderer");

		SetupCameraUniforms();

		m_image = std::make_unique<Image>("UV_Grid.jpg");

		m_texture_2d_handle = m_render_device->Create_Texture2D(m_image->Width(), m_image->Height(), render::VKRD::ImageUsage::TransferDstImage | render::VKRD::ImageUsage::SampledImage);
		if (m_texture_2d_handle != render::null_handle)
		{
		    m_render_device->UpdateTexture2DData(m_texture_2d_handle, m_image->Data(), m_image->DataSize(), {0, 0}, {m_image->Width(), m_image->Height()});
		}
    }

    SceneRenderer::~SceneRenderer()
    {
		for (RenderData& render_data : m_render_data)
		{
            DestroyBuffers(render_data);
		}
		m_render_device->DestroyTexture2D(m_texture_2d_handle);
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

    void SceneRenderer::BeginRender(uint32_t buffer_index, size_t current_frame)
    {
		m_current_buffer = buffer_index;
		m_current_frame = current_frame;
    }

    void SceneRenderer::UpdateDirtyInstances()
    {
		assert(m_render_device && "VulkanRenderDevice must be initialized on construction.");

		for (RenderData& render_data : m_render_data)
		{
			NodeComponent* node = render_data.m_owner_node;
			Transform3DComponent& transform = node->GetEntity().GetComponent<Transform3DComponent>();

			// update transformation uniform
			if (transform.Dirty() != Transform3DComponent::NOT_DIRTY || render_data.m_uniform_dirty[m_current_buffer])
			{
				if (!render_data.m_descriptor_sets[0])
				{
					Init_UniformBuffers(render_data);

                    render::DescriptorLayoutBuilder layoutBuilder = m_render_device->GetDescriptorLayoutBuilder();
					layoutBuilder
						.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
				        .SetBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

					render_data.m_descriptor_layout = layoutBuilder.BuildDescriptorLayout();

					std::array<vk::DescriptorBufferInfo, render::FRAME_LAG> model_descriptor_buffer_infos;
					for (uint32_t buffer_info_idx = 0; buffer_info_idx < render::FRAME_LAG; buffer_info_idx++)
					{
						model_descriptor_buffer_infos[buffer_info_idx] = render_data.m_uniform_buffers[buffer_info_idx].GetDescriptorInfo();
					}

					std::array<vk::DescriptorImageInfo, render::FRAME_LAG> model_descriptor_image_infos;
					for (uint32_t image_info_idx = 0; image_info_idx < render::FRAME_LAG; image_info_idx++)
					{
						model_descriptor_image_infos[image_info_idx] = m_render_device->GetImageDescriptorInfo(m_texture_2d_handle);
					}

					auto setBuilder = m_render_device->GetDescriptorSetBuilder(render_data.m_descriptor_layout);
					setBuilder.BindBuffer(0, model_descriptor_buffer_infos);
					setBuilder.BindImage(1, model_descriptor_image_infos);

					setBuilder.BuildDescriptorSet(render_data.m_descriptor_sets);

					BRR_LogInfo("Model Descriptor Sets created.");
				}

				if (transform.Dirty() != Transform3DComponent::NOT_DIRTY)
				{
					render_data.m_uniform_dirty.fill(true);
				}

				Mesh3DUniform uniform{};
				uniform.model_matrix = transform.GetGlobalTransform();

				render_data.m_uniform_buffers[m_current_buffer].Map();
				render_data.m_uniform_buffers[m_current_buffer].WriteToBuffer(&uniform, sizeof(uniform));
				render_data.m_uniform_buffers[m_current_buffer].Unmap();

				render_data.m_uniform_dirty[m_current_buffer] = false;
			}
		}

		UniformBufferObject ubo;
		ubo.projection_view = m_scene->GetMainCamera()->GetProjectionMatrix() * m_scene->GetMainCamera()->GetViewMatrix();;

		m_camera_uniform_info.m_uniform_buffers[m_current_buffer].Map();
		m_camera_uniform_info.m_uniform_buffers[m_current_buffer].WriteToBuffer(&ubo, sizeof(ubo));
		m_camera_uniform_info.m_uniform_buffers[m_current_buffer].Unmap();

    }

    void SceneRenderer::Render3D()
    {
		m_render_device->Bind_GraphicsPipeline();

		m_render_device->Bind_DescriptorSet(m_camera_uniform_info.m_descriptor_sets[m_current_buffer], 0);
		for (RenderData& render_data : m_render_data)
		{
			m_render_device->Bind_DescriptorSet(render_data.m_descriptor_sets[m_current_buffer], 1);

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
    }

    void SceneRenderer::SetupCameraUniforms()
    {
		for (size_t idx = 0; idx < render::FRAME_LAG; idx++)
		{
			m_camera_uniform_info.m_uniform_buffers[idx] =
                render::DeviceBuffer(sizeof(UniformBufferObject),
					                 render::VulkanRenderDevice::BufferUsage::UniformBuffer,
                                     render::VKRD::AUTO,
                                     VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		}

        render::DescriptorLayoutBuilder layoutBuilder = m_render_device->GetDescriptorLayoutBuilder();
		layoutBuilder
			.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

		std::array<vk::DescriptorBufferInfo, render::FRAME_LAG> camera_descriptor_buffer_infos;
		for (uint32_t buffer_info_idx = 0; buffer_info_idx < render::FRAME_LAG; buffer_info_idx++)
		{
			camera_descriptor_buffer_infos[buffer_info_idx] =
				m_camera_uniform_info.m_uniform_buffers[buffer_info_idx].GetDescriptorInfo();
		}

        render::DescriptorLayout layout = layoutBuilder.BuildDescriptorLayout();
		auto setBuilder = m_render_device->GetDescriptorSetBuilder(layout);
		setBuilder.BindBuffer(0, camera_descriptor_buffer_infos);

		setBuilder.BuildDescriptorSet(m_camera_uniform_info.m_descriptor_sets);
    }

    void SceneRenderer::CreateVertexBuffer(std::vector<Vertex3>& vertex_buffer, RenderData& render_data)
    {
		assert(!vertex_buffer.empty() && "Vertices data can't be empty.");

        const size_t buffer_size = sizeof(Vertex3) * vertex_buffer.size();

		BRR_LogInfo("Creating Vertex Buffer.");

		render::VulkanRenderDevice::VertexFormatFlags vertex_format = render::VulkanRenderDevice::VertexFormatFlags::COLOR;

        render_data.m_vertex_buffer_handle = m_render_device->CreateVertexBuffer(buffer_size, vertex_format, vertex_buffer.data());

		render_data.num_vertices = vertex_buffer.size();
    }

    void SceneRenderer::CreateIndexBuffer(std::vector<uint32_t>& index_buffer, RenderData& render_data)
    {
		if (index_buffer.empty())
			return;

		vk::DeviceSize buffer_size = sizeof(uint32_t) * index_buffer.size();

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

    void SceneRenderer::Init_UniformBuffers(RenderData& render_data)
	{
		vk::DeviceSize buffer_size = sizeof(Mesh3DUniform);

		BRR_LogInfo("Creating Uniform Buffers");
		for (uint32_t i = 0; i < render::FRAME_LAG; i++)
		{
            render_data.m_uniform_buffers[i] = render::DeviceBuffer(buffer_size,
                                                                    render::VKRD::BufferUsage::UniformBuffer,
                                                                    render::VKRD::AUTO,
                                                                    VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		}

		BRR_LogInfo("Uniform Buffers created.");
	}
}
