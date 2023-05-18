#include "Renderer/SceneRenderer.h"
#include "Renderer/RenderDevice.h"

#include "Scene/Components/Mesh3DComponent.h"
#include "Scene/Components/Transform3DComponent.h"
#include "Core/LogSystem.h"

namespace brr::render
{
	
	static SurfaceId GetNewSurfaceId()
	{
		static std::atomic<uint64_t> current_surface_id = 0;
		return static_cast<SurfaceId>(current_surface_id++);
	}

    SceneRenderer::SceneRenderer(RenderDevice* device, Scene* scene)
    : m_scene(scene),
      m_render_device(device)
    {
		assert(m_render_device && "Can't create SceneRenderer without RenderDevice.");
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
		UpdateBufferData(render_data.m_vertex_buffer, vertex_buffer.data(), vertex_buffer.size() * sizeof(Vertex3_PosColor), buffer_offset);
		//CreateVertexBuffer(vertex_buffer, render_data);
    }

    void SceneRenderer::UpdateSurfaceIndexBuffer(SurfaceId surface_id, std::vector<uint32_t>& index_buffer, uint32_t buffer_offset)
    {
		uint32_t surf_index = m_surfId_idx_map.at(surface_id);
		assert(m_surfId_idx_map.contains(surface_id) && "Surface is not in the render data map. Something went wrong.");
		RenderData& render_data = m_render_data[surf_index];
		UpdateBufferData(render_data.m_index_buffer, index_buffer.data(), index_buffer.size() * sizeof(uint32_t), buffer_offset);
		//CreateIndexBuffer(index_buffer, render_data);
    }

    void SceneRenderer::UpdateDirtyInstances(uint32_t buffer_index)
    {
		assert(m_render_device && "RenderDevice must be initialized on construction.");

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
			if (transform.Dirty() != Transform3DComponent::NOT_DIRTY || render_data.m_uniform_dirty[buffer_index])
			{
				if (!render_data.m_descriptor_sets[0])
				{
					Init_UniformBuffers(render_data);

					DescriptorLayoutBuilder layoutBuilder = m_render_device->GetDescriptorLayoutBuilder();
					layoutBuilder
						.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

					render_data.m_descriptor_layout = layoutBuilder.BuildDescriptorLayout();

					std::array<vk::DescriptorBufferInfo, FRAME_LAG> model_descriptor_buffer_infos;
					for (uint32_t buffer_info_idx = 0; buffer_info_idx < FRAME_LAG; buffer_info_idx++)
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

				render_data.m_uniform_buffers[buffer_index].Map();
				render_data.m_uniform_buffers[buffer_index].WriteToBuffer(&uniform, sizeof(uniform));
				render_data.m_uniform_buffers[buffer_index].Unmap();

				render_data.m_uniform_dirty[buffer_index] = false;
			}
		}

		UniformBufferObject ubo;
		ubo.projection_view = m_scene->GetMainCamera()->GetProjectionMatrix() * m_scene->GetMainCamera()->GetViewMatrix();;

		m_camera_uniform_info.m_uniform_buffers[buffer_index].Map();
		m_camera_uniform_info.m_uniform_buffers[buffer_index].WriteToBuffer(&ubo, sizeof(ubo));
		m_camera_uniform_info.m_uniform_buffers[buffer_index].Unmap();

    }

    void SceneRenderer::Render3D(vk::CommandBuffer cmd_buffer, uint32_t buffer_index, const DevicePipeline& render_pipeline)
    {
		cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.GetPipeline());

		cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_pipeline.GetPipelineLayout(), 0, m_camera_uniform_info.m_descriptor_sets[buffer_index], {});

		for (RenderData& render_data : m_render_data)
		{
			cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_pipeline.GetPipelineLayout(), 1, render_data.m_descriptor_sets[buffer_index], {});

			assert(render_data.m_vertex_buffer.IsValid() && "Vertex buffer must be valid to bind to a command buffer.");
			cmd_buffer.bindVertexBuffers(0, render_data.m_vertex_buffer.GetBuffer(), { 0 });

			if (render_data.m_index_buffer.IsValid())
			{
				cmd_buffer.bindIndexBuffer(render_data.m_index_buffer.GetBuffer(), 0, vk::IndexType::eUint32);
				cmd_buffer.drawIndexed(render_data.num_indices, 1, 0, 0, 0);
			}
			else
			{
				cmd_buffer.draw( render_data.num_vertices, 1, 0, 0);
			}
		}
    }

    void SceneRenderer::SetupCameraUniforms()
    {
		for (size_t idx = 0; idx < FRAME_LAG; idx++)
		{
			m_camera_uniform_info.m_uniform_buffers[idx] =
				DeviceBuffer(*m_render_device, sizeof(UniformBufferObject),
					vk::BufferUsageFlagBits::eUniformBuffer,
					vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		}

		DescriptorLayoutBuilder layoutBuilder = m_render_device->GetDescriptorLayoutBuilder();
		layoutBuilder
			.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

		std::array<vk::DescriptorBufferInfo, FRAME_LAG> camera_descriptor_buffer_infos;
		for (uint32_t buffer_info_idx = 0; buffer_info_idx < FRAME_LAG; buffer_info_idx++)
		{
			camera_descriptor_buffer_infos[buffer_info_idx] =
				m_camera_uniform_info.m_uniform_buffers[buffer_info_idx].GetDescriptorInfo();
		}

		DescriptorLayout layout = layoutBuilder.BuildDescriptorLayout();
		auto setBuilder = m_render_device->GetDescriptorSetBuilder(layout);
		setBuilder.BindBuffer(0, camera_descriptor_buffer_infos);

		setBuilder.BuildDescriptorSet(m_camera_uniform_info.m_descriptor_sets);
    }

    void SceneRenderer::CreateVertexBuffer(std::vector<Vertex3_PosColor>& vertex_buffer, RenderData& render_data)
    {
		assert(!vertex_buffer.empty() && "Vertices data can't be empty.");

		vk::DeviceSize buffer_size = sizeof(Vertex3_PosColor) * vertex_buffer.size();

		render::DeviceBuffer staging_buffer = CreateStagingBuffer(buffer_size, vertex_buffer.data());

		BRR_LogInfo("Vertices data copied to Staging Buffer.");

		BRR_LogInfo("Creating Vertex Buffer.");

		render_data.m_vertex_buffer = DeviceBuffer(*m_render_device, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		render_data.num_vertices = vertex_buffer.size();

		BRR_LogInfo("Copying Staging Buffer into Vertex Buffer.");

		m_render_device->Copy_Buffer_Immediate(staging_buffer.GetBuffer(), render_data.m_vertex_buffer.GetBuffer(), buffer_size);

		BRR_LogInfo("Destroying Staging Buffer.");
    }

    void SceneRenderer::CreateIndexBuffer(std::vector<uint32_t>& index_buffer, RenderData& render_data)
    {
		if (index_buffer.empty())
			return;

		vk::DeviceSize buffer_size = sizeof(uint32_t) * index_buffer.size();

		render::DeviceBuffer staging_buffer = CreateStagingBuffer(buffer_size, index_buffer.data());

		BRR_LogInfo("Indices data copied to Staging Buffer.");

		BRR_LogInfo("Creating Index Buffer.");

		render_data.m_index_buffer = DeviceBuffer(*m_render_device, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		render_data.num_indices = index_buffer.size();

		BRR_LogInfo("Copying Staging Buffer into Index Buffer.");

		m_render_device->Copy_Buffer_Immediate(staging_buffer.GetBuffer(), render_data.m_index_buffer.GetBuffer(), buffer_size);

		BRR_LogInfo("Destroying Staging Buffer.");
    }

    void SceneRenderer::UpdateBufferData(DeviceBuffer& buffer, void* data, uint32_t size, uint32_t offset)
    {
		BRR_LogInfo("Updating buffer data.");
		render::DeviceBuffer staging_buffer = CreateStagingBuffer(size, data);

		m_render_device->Copy_Buffer_Immediate(staging_buffer.GetBuffer(), buffer.GetBuffer(), size, 0, offset);
    }

    DeviceBuffer SceneRenderer::CreateStagingBuffer(vk::DeviceSize buffer_size, void* buffer_data)
    {
		BRR_LogInfo("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ *m_render_device,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(buffer_data);

		return staging_buffer;
    }

    void SceneRenderer::Init_UniformBuffers(RenderData& render_data)
	{
		vk::DeviceSize buffer_size = sizeof(Mesh3DUniform);

		BRR_LogInfo("Creating Uniform Buffers");
		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			render_data.m_uniform_buffers[i].Reset(*m_render_device, buffer_size,
                                                   vk::BufferUsageFlagBits::eUniformBuffer,
                                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		}

		BRR_LogInfo("Uniform Buffers created.");
	}
}
