#include "SceneRenderer.h"

#include <Renderer/VulkanRenderDevice.h>
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

    void SceneRenderer::UpdateSurfaceVertexBuffer(SurfaceId surface_id, std::vector<Vertex3_PosColor>& vertex_buffer, uint32_t buffer_offset)
    {
		uint32_t surf_index = m_surfId_idx_map.at(surface_id);
		assert(m_surfId_idx_map.contains(surface_id) && "Surface is not in the render data map. Something went wrong.");
		RenderData& render_data = m_render_data[surf_index];
		size_t buffer_size = vertex_buffer.size() * sizeof(Vertex3_PosColor);
		UpdateBufferData(render_data.m_vertex_buffer, vertex_buffer.data(), buffer_size, buffer_offset);

		if (m_render_device->IsDifferentTransferQueue())
		{
			vk::BufferMemoryBarrier2 buffer_memory_barrier {};
			buffer_memory_barrier
				.setBuffer(render_data.m_vertex_buffer.GetBuffer())
				.setSize(buffer_size)
				.setSrcQueueFamilyIndex(m_render_device->GetQueueFamilyIndices().m_transferFamily.value())
				.setDstQueueFamilyIndex(m_render_device->GetQueueFamilyIndices().m_graphicsFamily.value())
				.setDstStageMask(vk::PipelineStageFlagBits2::eVertexAttributeInput)
				.setDstAccessMask(vk::AccessFlagBits2::eVertexAttributeRead);

			vk::DependencyInfo dependency_info {};
			dependency_info
				.setBufferMemoryBarriers(buffer_memory_barrier);

			m_current_graphics_cmd_buffer.pipelineBarrier2(dependency_info);
		}
    }

    void SceneRenderer::UpdateSurfaceIndexBuffer(SurfaceId surface_id, std::vector<uint32_t>& index_buffer, uint32_t buffer_offset)
    {
		uint32_t surf_index = m_surfId_idx_map.at(surface_id);
		assert(m_surfId_idx_map.contains(surface_id) && "Surface is not in the render data map. Something went wrong.");
		RenderData& render_data = m_render_data[surf_index];
		size_t buffer_size = index_buffer.size() * sizeof(uint32_t);
		UpdateBufferData(render_data.m_index_buffer, index_buffer.data(), buffer_size, buffer_offset);

		if (m_render_device->IsDifferentTransferQueue())
		{
			vk::BufferMemoryBarrier2 buffer_memory_barrier {};
			buffer_memory_barrier
				.setBuffer(render_data.m_index_buffer.GetBuffer())
			    .setSize(buffer_size)
				.setSrcQueueFamilyIndex(m_render_device->GetQueueFamilyIndices().m_transferFamily.value())
				.setDstQueueFamilyIndex(m_render_device->GetQueueFamilyIndices().m_graphicsFamily.value())
				.setDstStageMask(vk::PipelineStageFlagBits2::eIndexInput)
				.setDstAccessMask(vk::AccessFlagBits2::eIndexRead);

			vk::DependencyInfo dependency_info {};
			dependency_info
				.setBufferMemoryBarriers(buffer_memory_barrier);

			m_current_graphics_cmd_buffer.pipelineBarrier2(dependency_info);
		}
    }

    void SceneRenderer::BeginRender(uint32_t buffer_index, size_t current_frame, vk::CommandBuffer graphics_command_buffer,
        vk::CommandBuffer transfer_command_buffer)
    {
		m_current_buffer = buffer_index;
		m_current_frame = current_frame;
		m_current_graphics_cmd_buffer = graphics_command_buffer;
		m_current_transfer_cmd_buffer = transfer_command_buffer;
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

                        UpdateSurfaceVertexBuffer(static_cast<SurfaceId>(surface.GetRenderSurfaceID()), vertices, 0);
                        UpdateSurfaceIndexBuffer(static_cast<SurfaceId>(surface.GetRenderSurfaceID()), indices, 0);
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

			assert(render_data.m_vertex_buffer.IsInitialized() && "Vertex buffer must be valid to bind to a command buffer.");
			m_current_graphics_cmd_buffer.bindVertexBuffers(0, render_data.m_vertex_buffer.GetBuffer(), { 0 });

			if (render_data.m_index_buffer.IsInitialized())
			{
				m_current_graphics_cmd_buffer.bindIndexBuffer(render_data.m_index_buffer.GetBuffer(), 0, vk::IndexType::eUint32);
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
                                     vk::BufferUsageFlagBits::eUniformBuffer,
                                     VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
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

		BRR_LogInfo("Vertices data copied to Staging Buffer.");

		BRR_LogInfo("Creating Vertex Buffer.");

        render_data.m_vertex_buffer = render::DeviceBuffer(buffer_size,
                                                           vk::BufferUsageFlagBits::eTransferDst |
                                                           vk::BufferUsageFlagBits::eVertexBuffer,
                                                           VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO);

		render_data.num_vertices = vertex_buffer.size();

		BRR_LogInfo("Copying Staging Buffer into Vertex Buffer.");

		UpdateBufferData(render_data.m_vertex_buffer, vertex_buffer.data(), buffer_size, 0);

		if (m_render_device->IsDifferentTransferQueue())
		{
			vk::BufferMemoryBarrier2 buffer_memory_barrier {};
			buffer_memory_barrier
				.setBuffer(render_data.m_vertex_buffer.GetBuffer())
				.setSize(buffer_size)
				.setSrcQueueFamilyIndex(m_render_device->GetQueueFamilyIndices().m_transferFamily.value())
				.setDstQueueFamilyIndex(m_render_device->GetQueueFamilyIndices().m_graphicsFamily.value())
				.setDstStageMask(vk::PipelineStageFlagBits2::eVertexAttributeInput)
				.setDstAccessMask(vk::AccessFlagBits2::eVertexAttributeRead);

			vk::DependencyInfo dependency_info {};
			dependency_info
				.setBufferMemoryBarriers(buffer_memory_barrier);

			m_current_graphics_cmd_buffer.pipelineBarrier2(dependency_info);
		}

		BRR_LogInfo("Destroying Staging Buffer.");
    }

    void SceneRenderer::CreateIndexBuffer(std::vector<uint32_t>& index_buffer, RenderData& render_data)
    {
		if (index_buffer.empty())
			return;

		vk::DeviceSize buffer_size = sizeof(uint32_t) * index_buffer.size();

		BRR_LogInfo("Indices data copied to Staging Buffer.");

		BRR_LogInfo("Creating Index Buffer.");

        render_data.m_index_buffer = render::DeviceBuffer(buffer_size,
                                                          vk::BufferUsageFlagBits::eTransferDst |
                                                          vk::BufferUsageFlagBits::eIndexBuffer,
                                                          VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO);

		render_data.num_indices = index_buffer.size();

		BRR_LogInfo("Copying Staging Buffer into Index Buffer.");

		UpdateBufferData(render_data.m_index_buffer, index_buffer.data(), buffer_size, 0);

		if (m_render_device->IsDifferentTransferQueue())
		{
		    vk::BufferMemoryBarrier2 buffer_memory_barrier {};
		    buffer_memory_barrier
                .setBuffer(render_data.m_index_buffer.GetBuffer())
                .setSize(buffer_size)
                .setSrcQueueFamilyIndex(m_render_device->GetQueueFamilyIndices().m_transferFamily.value())
                .setDstQueueFamilyIndex(m_render_device->GetQueueFamilyIndices().m_graphicsFamily.value())
		        .setDstStageMask(vk::PipelineStageFlagBits2::eIndexInput)
		        .setDstAccessMask(vk::AccessFlagBits2::eIndexRead);

			vk::DependencyInfo dependency_info {};
			dependency_info
				.setBufferMemoryBarriers(buffer_memory_barrier);

			m_current_graphics_cmd_buffer.pipelineBarrier2(dependency_info);
		}

		BRR_LogInfo("Destroying Staging Buffer.");
    }

    void SceneRenderer::UpdateBufferData(render::DeviceBuffer& buffer, void* data, uint32_t size, uint32_t offset)
    {
		BRR_LogInfo("Updating buffer data.");
        render::StagingBufferHandle staging_buffer = CreateStagingBuffer(size, data);

		m_staging_allocator.CopyFromStagingToBuffer(staging_buffer, buffer, m_current_transfer_cmd_buffer, size);
    }

    render::StagingBufferHandle SceneRenderer::CreateStagingBuffer(size_t buffer_size, void* buffer_data)
    {
		BRR_LogInfo("Creating Staging Buffer.");

        render::StagingBufferHandle staging_buffer{};
		m_staging_allocator.AllocateStagingBuffer(m_current_frame, buffer_size, &staging_buffer);

		m_staging_allocator.WriteToStagingBuffer(staging_buffer, 0, buffer_data, buffer_size);

		return staging_buffer;
    }

    void SceneRenderer::Init_UniformBuffers(RenderData& render_data)
	{
		vk::DeviceSize buffer_size = sizeof(Mesh3DUniform);

		BRR_LogInfo("Creating Uniform Buffers");
		for (uint32_t i = 0; i < render::FRAME_LAG; i++)
		{
			render_data.m_uniform_buffers[i].Reset(buffer_size, vk::BufferUsageFlagBits::eUniformBuffer,
                                                   VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
				                                   VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		}

		BRR_LogInfo("Uniform Buffers created.");
	}
}
