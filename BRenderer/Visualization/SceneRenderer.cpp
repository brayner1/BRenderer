#include "SceneRenderer.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Scene/Components/Transform3DComponent.h>
#include <Core/LogSystem.h>

namespace brr::vis
{
	
	static SurfaceId GetNewSurfaceId()
	{
		static std::atomic<uint64_t> current_surface_id = 0;
		return static_cast<SurfaceId>(current_surface_id++);
	}

    SceneRenderer::SceneRenderer(Scene* scene)
    : m_scene(scene),
      m_render_device(render::VKRD::GetSingleton())
    {
		assert(m_render_device && "Can't create SceneRenderer without VulkanRenderDevice.");
		BRR_LogInfo("Creating SceneRenderer");

		auto dirty_meshes_group = m_scene->m_registry_.group<Mesh3DComponent, MeshDirty>();

		auto mesh_view = m_scene->m_registry_.view<Mesh3DComponent>();

		SetupCameraUniforms();
    }

    SceneRenderer::~SceneRenderer()
    {
		for (RenderData& render_data : m_render_data)
		{
		    m_render_device->DestroyVertexBuffer(render_data.m_vertex_buffer_handle);
		    m_render_device->DestroyIndexBuffer(render_data.m_index_buffer_handle);
		}
    }

    SurfaceId SceneRenderer::CreateNewSurface(Mesh3DComponent::SurfaceData& surface,
                                              const Entity& owner_entity)
	{
		SurfaceId surface_id = GetNewSurfaceId();
		surface.SetRenderSurfaceID(static_cast<uint64_t>(surface_id));
		BRR_LogInfo("Adding new RenderData for Surface with ID: {}", static_cast<uint64_t>(surface_id));
		
		m_surfId_idx_map.emplace(surface_id, m_render_data.size());
		NodeComponent& owner_node = owner_entity.GetComponent<NodeComponent>();
		RenderData& render_data = m_render_data.emplace_back(&owner_node);

		auto& vertices = const_cast<std::vector<Vertex3_PosColor>&> (surface.GetVertices());
		auto& indices = const_cast<std::vector<uint32_t>&> (surface.GetIndices());

		CreateVertexBuffer(vertices, render_data);
		CreateIndexBuffer(indices, render_data);

		return surface_id;
	}

    void SceneRenderer::BeginRender(uint32_t buffer_index, size_t current_frame)
    {
		m_current_buffer = buffer_index;
		m_current_frame = current_frame;
		m_current_graphics_cmd_buffer = m_render_device->GetCurrentGraphicsCommandBuffer();
		m_current_transfer_cmd_buffer = m_render_device->GetCurrentTransferCommandBuffer();
    }

    void SceneRenderer::UpdateDirtyInstances()
    {
		assert(m_render_device && "VulkanRenderDevice must be initialized on construction.");

		bool updated_render_data = false;

		auto dirty_meshes_group = m_scene->m_registry_.group<Mesh3DComponent, MeshDirty>();
		dirty_meshes_group.each([&](auto entity, Mesh3DComponent& mesh_component)
		{
			BRR_LogInfo("Updating Dirty Mesh Component of entity {}", (uint64_t)entity);
			for (const uint32_t& index : mesh_component.m_dirty_surfaces)
			{
				Mesh3DComponent::SurfaceData& surface = mesh_component.m_surfaces[index];
			    if (surface.isDirty())
			    {
					SurfaceId surface_id = static_cast<SurfaceId>(surface.GetRenderSurfaceID());
					if (!m_surfId_idx_map.contains(surface_id) || surface_id == SurfaceId::NULL_ID)
					{
						Entity owner_entity{ entity, m_scene };
						CreateNewSurface(surface, owner_entity);
					}
					else
					{
						if (!updated_render_data)
						{
							m_render_device->GetGraphicsQueue().waitIdle();
						}
					    auto& vertices = const_cast<std::vector<Vertex3_PosColor>&> (surface.GetVertices());
                        auto& indices = const_cast<std::vector<uint32_t>&> (surface.GetIndices());

						uint32_t surf_index = m_surfId_idx_map.at(surface_id);
		                assert(m_surfId_idx_map.contains(surface_id) && "Surface is not in the render data map. Something went wrong.");
		                RenderData& render_data = m_render_data[surf_index];

						m_render_device->UpdateVertexBufferData(render_data.m_vertex_buffer_handle, vertices.data(), vertices.size() * sizeof(Vertex3_PosColor), 0);
						m_render_device->UpdateIndexBufferData(render_data.m_index_buffer_handle, indices.data(), indices.size() * sizeof(uint32_t), 0);
					}
					surface.SetIsDirty(false);
			    }
			}
			mesh_component.m_dirty_surfaces.clear();
		});

		m_scene->m_registry_.clear<MeshDirty>();

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
						.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

					render_data.m_descriptor_layout = layoutBuilder.BuildDescriptorLayout();

					std::array<vk::DescriptorBufferInfo, render::FRAME_LAG> model_descriptor_buffer_infos;
					for (uint32_t buffer_info_idx = 0; buffer_info_idx < render::FRAME_LAG; buffer_info_idx++)
					{
						model_descriptor_buffer_infos[buffer_info_idx] = render_data.m_uniform_buffers[buffer_info_idx].GetDescriptorInfo();
					}

					auto setBuilder = m_render_device->GetDescriptorSetBuilder(render_data.m_descriptor_layout);
					setBuilder.BindBuffer(0, model_descriptor_buffer_infos);

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

    void SceneRenderer::Render3D(const render::DevicePipeline& render_pipeline)
    {
		m_current_graphics_cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.GetPipeline());

		m_current_graphics_cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_pipeline.GetPipelineLayout(), 0, m_camera_uniform_info.m_descriptor_sets[m_current_buffer], {});

		for (RenderData& render_data : m_render_data)
		{
			m_current_graphics_cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_pipeline.GetPipelineLayout(), 1, render_data.m_descriptor_sets[m_current_buffer], {});

			assert(render_data.m_vertex_buffer_handle.IsValid() && "Vertex buffer must be valid to bind to a command buffer.");
			m_render_device->BindVertexBuffer(render_data.m_vertex_buffer_handle);

			if (render_data.m_index_buffer_handle.IsValid())
			{
				m_render_device->BindIndexBuffer(render_data.m_index_buffer_handle);
				m_current_graphics_cmd_buffer.drawIndexed(render_data.num_indices, 1, 0, 0, 0);
			}
			else
			{
				m_current_graphics_cmd_buffer.draw( render_data.num_vertices, 1, 0, 0);
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

    void SceneRenderer::CreateVertexBuffer(std::vector<Vertex3_PosColor>& vertex_buffer, RenderData& render_data)
    {
		assert(!vertex_buffer.empty() && "Vertices data can't be empty.");

		vk::DeviceSize buffer_size = sizeof(Vertex3_PosColor) * vertex_buffer.size();

		BRR_LogInfo("Creating Vertex Buffer.");

		render::VulkanRenderDevice::VertexFormatFlags vertex_format = render::VulkanRenderDevice::VertexFormatFlags::COLOR;

        render_data.m_vertex_buffer_handle = m_render_device->CreateVertexBuffer(vertex_buffer.data(), buffer_size, vertex_format);

		render_data.num_vertices = vertex_buffer.size();
    }

    void SceneRenderer::CreateIndexBuffer(std::vector<uint32_t>& index_buffer, RenderData& render_data)
    {
		if (index_buffer.empty())
			return;

		vk::DeviceSize buffer_size = sizeof(uint32_t) * index_buffer.size();

		BRR_LogInfo("Creating Index Buffer.");

        render::VulkanRenderDevice::IndexType index_type = render::VulkanRenderDevice::IndexType::UINT32;

        render_data.m_index_buffer_handle = m_render_device->CreateIndexBuffer(index_buffer.data(), buffer_size, index_type);

		render_data.num_indices = index_buffer.size();
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
