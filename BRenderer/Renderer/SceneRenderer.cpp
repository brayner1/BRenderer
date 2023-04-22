#include "Renderer/SceneRenderer.h"

#include "Scene/Scene.h"
#include "Scene/Components/Mesh3DComponent.h"
#include "Scene/Components/Transform3DComponent.h"
#include "Core/LogSystem.h"

namespace brr::render
{
    SceneRenderer::SceneRenderer(entt::registry& registry)
    : m_render_device(Renderer::GetRenderer()->GetDevice())
    {
		assert(m_render_device && "Can't create SceneRenderer without RenderDevice.");
		BRR_LogInfo("Creating SceneRenderer");
        registry.on_construct<Mesh3DComponent>().connect<&SceneRenderer::OnAddedMesh3D>(this);

		Renderer* renderer = Renderer::GetRenderer();

		for (size_t idx = 0; idx < FRAME_LAG; idx++)
		{
			m_camera_uniform_info.m_uniform_buffers[idx] = 
				DeviceBuffer(m_render_device->Get_VkDevice(), sizeof(UniformBufferObject),
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		}

		DescriptorLayoutBuilder layoutBuilder = renderer->GetDescriptorLayoutBuilder();
		layoutBuilder
			.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

		std::array<vk::DescriptorBufferInfo, FRAME_LAG> camera_descriptor_buffer_infos;
		for (uint32_t buffer_info_idx = 0; buffer_info_idx < FRAME_LAG; buffer_info_idx++)
		{
			camera_descriptor_buffer_infos[buffer_info_idx] =
				m_camera_uniform_info.m_uniform_buffers[buffer_info_idx].GetDescriptorInfo();
		}

		DescriptorLayout layout = layoutBuilder.BuildDescriptorLayout();
		auto setBuilder = renderer->GetDescriptorSetBuilder(layout);
		setBuilder.BindBuffer(0, camera_descriptor_buffer_infos);

		setBuilder.BuildDescriptorSet(m_camera_uniform_info.m_descriptor_sets);
    }

    void SceneRenderer::UpdateRenderData(entt::registry& scene_registry, uint32_t buffer_index, glm::mat4& projection_view)
    {
		assert(m_render_device && "RenderDevice must be initialized on construction.");
		auto render_group = scene_registry.group<Transform3DComponent, Mesh3DComponent>();

		Renderer* renderer = Renderer::GetRenderer();
		render_group.each([&](auto entity, Transform3DComponent& transform, Mesh3DComponent& mesh)
		{
			for (Mesh3DComponent::SurfaceData& surface : mesh)
			{
			    if (!surface.NeedUpdate() && (transform.Dirty() != Transform3DComponent::NOT_DIRTY))
			    {
					continue;
			    }

                SurfaceId surf_id = static_cast<SurfaceId>(surface.GetSurfaceID());
				assert(m_render_registry.valid(surf_id) && "Surface is not in the render registry. Something went wrong.");
				RenderData& render_data = m_render_registry.get<RenderData>(surf_id);

				// Update vertex and index buffer
				if (surface.NeedUpdate())
			    {
			        CreateVertexBuffer(surface, render_data);
                    CreateIndexBuffer(surface, render_data);

					surface.SetUpdated();
			    }

				// update transformation uniform
				if (transform.Dirty() != Transform3DComponent::NOT_DIRTY || render_data.m_uniform_dirty[buffer_index])
				{
					if (!render_data.m_descriptor_sets[0])
					{
						Init_UniformBuffers(render_data);

						DescriptorLayoutBuilder layoutBuilder = renderer->GetDescriptorLayoutBuilder();
						layoutBuilder
							.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

						render_data.m_descriptor_layout = layoutBuilder.BuildDescriptorLayout();

                        std::array<vk::DescriptorBufferInfo, FRAME_LAG> model_descriptor_buffer_infos;
                        for (uint32_t buffer_info_idx = 0; buffer_info_idx < FRAME_LAG; buffer_info_idx++)
                        {
                           model_descriptor_buffer_infos[buffer_info_idx] = render_data.m_uniform_buffers[buffer_info_idx].GetDescriptorInfo();
                        }

                        auto setBuilder = renderer->GetDescriptorSetBuilder(render_data.m_descriptor_layout);
					    setBuilder.BindBuffer(0, model_descriptor_buffer_infos);

                        setBuilder.BuildDescriptorSet(render_data.m_descriptor_sets);

                        BRR_LogInfo("Model Descriptor Sets created.");
					}

					if (transform.Dirty() != Transform3DComponent::NOT_DIRTY)
					{
						render_data.m_uniform_dirty.fill(true);
					}

					Mesh3DUniform uniform {};
					uniform.model_matrix = transform.GetGlobalTransform();

					render_data.m_uniform_buffers[buffer_index].Map();
					render_data.m_uniform_buffers[buffer_index].WriteToBuffer(&uniform, sizeof(uniform));
					render_data.m_uniform_buffers[buffer_index].Unmap();

					render_data.m_uniform_dirty[buffer_index] = false;
				}

				UniformBufferObject ubo;
				ubo.projection_view = projection_view;

				m_camera_uniform_info.m_uniform_buffers[buffer_index].Map();
				m_camera_uniform_info.m_uniform_buffers[buffer_index].WriteToBuffer(&ubo, sizeof(ubo));
				m_camera_uniform_info.m_uniform_buffers[buffer_index].Unmap();

			}
		});

    }

    void SceneRenderer::Render(vk::CommandBuffer cmd_buffer, uint32_t buffer_index, const DevicePipeline& render_pipeline)
    {
		cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.GetPipeline());

		cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_pipeline.GetPipelineLayout(), 0, m_camera_uniform_info.m_descriptor_sets[buffer_index], {});

		auto render_view = m_render_registry.view<RenderData>();

		render_view.each([&](auto entity, RenderData& render_data)
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
		});
    }

    void SceneRenderer::OnAddedMesh3D(entt::registry& registry, entt::entity entity)
    {
        Mesh3DComponent& mesh_3d_component = registry.get<Mesh3DComponent>(entity);
		BRR_LogInfo("Mesh3DComponent added to scene. Creating its RenderData.");

		for (Mesh3DComponent::SurfaceData& surface : mesh_3d_component)
		{
            auto surf_id = static_cast<SurfaceId>(surface.GetSurfaceID());
			if (!m_render_registry.valid(surf_id))
			{
				m_render_registry.create(surf_id);
				m_render_registry.emplace<RenderData>(surf_id);
			}
		}
    }

    void SceneRenderer::CreateVertexBuffer(Mesh3DComponent::SurfaceData& surface_data, RenderData& render_data)
    {
		assert(!surface_data.m_vertices.empty() && "Vertices data can't be empty.");
		render::Renderer* render = render::Renderer::GetRenderer();
		render::RenderDevice* render_device = render->GetDevice();
		vk::Device vkDevice = render_device->Get_VkDevice();

		vk::DeviceSize buffer_size = sizeof(Vertex3_PosColor) * surface_data.m_vertices.size();

		BRR_LogInfo("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ vkDevice,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(surface_data.m_vertices.data());
		//staging_buffer.Unmap();

		BRR_LogInfo("Vertices data copied to Staging Buffer.");

		BRR_LogInfo("Creating Vertex Buffer.");

		render_data.m_vertex_buffer = DeviceBuffer(vkDevice, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		render_data.num_vertices = surface_data.m_vertices.size();

		BRR_LogInfo("Copying Staging Buffer into Vertex Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), render_data.m_vertex_buffer.GetBuffer(), buffer_size);

		BRR_LogInfo("Destroying Staging Buffer.");
    }

    void SceneRenderer::CreateIndexBuffer(Mesh3DComponent::SurfaceData& surface_data, RenderData& render_data)
    {
		if (surface_data.m_indices.empty())
			return;

		render::Renderer* render = render::Renderer::GetRenderer();
		render::RenderDevice* render_device = render->GetDevice();
		vk::Device vkDevice = render_device->Get_VkDevice();

		vk::DeviceSize buffer_size = sizeof(uint32_t) * surface_data.m_indices.size();

		BRR_LogInfo("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ vkDevice,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(surface_data.m_indices.data());

		BRR_LogInfo("Indices data copied to Staging Buffer.");

		BRR_LogInfo("Creating Index Buffer.");

		render_data.m_index_buffer = DeviceBuffer(vkDevice, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		render_data.num_indices = surface_data.m_indices.size();

		BRR_LogInfo("Copying Staging Buffer into Index Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), render_data.m_index_buffer.GetBuffer(), buffer_size);

		BRR_LogInfo("Destroying Staging Buffer.");
    }

    void SceneRenderer::Init_UniformBuffers(RenderData& render_data)
	{
		vk::DeviceSize buffer_size = sizeof(Mesh3DUniform);

		BRR_LogInfo("Creating Uniform Buffers");
		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			render_data.m_uniform_buffers[i].Reset(m_render_device->Get_VkDevice(), buffer_size,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		}

		BRR_LogInfo("Uniform Buffers created.");
	}
}
