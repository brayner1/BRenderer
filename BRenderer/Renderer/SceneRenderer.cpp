#include "SceneRenderer.h"

#include "Renderer/Renderer.h"

#include "Scene/Components/Mesh3DComponent.h"
#include "Scene/Components/Transform3DComponent.h"

namespace brr::render
{
    SceneRenderer::SceneRenderer(entt::registry& registry)
    : m_registry(&registry)
    {
		SDL_Log("Creating SceneRenderer");
        registry.on_construct<Mesh3DComponent>().connect<&SceneRenderer::OnAddedMesh3D>(this);
    }

    void SceneRenderer::OnAddedMesh3D(entt::registry& registry, entt::entity entity)
    {
        Mesh3DComponent& mesh_3d_component = registry.get<Mesh3DComponent>(entity);
		SDL_Log("OnAddedMesh3D");

		for (Mesh3DComponent::SurfaceData& surface : mesh_3d_component.surfaces)
		{
			SDL_Log("Configuring surface");
			CreateVertexBuffer(surface);
			CreateIndexBuffer(surface);
		}
    }

    void SceneRenderer::CreateVertexBuffer(Mesh3DComponent::SurfaceData& surface_data)
    {
		assert(!surface_data.m_vertices.empty() && "Vertices data can't be empty.");
		render::Renderer* render = render::Renderer::GetRenderer();
		render::RenderDevice* render_device = render->GetDevice();
		vk::Device vkDevice = render_device->Get_VkDevice();

		vk::DeviceSize buffer_size = sizeof(Vertex3_PosColor) * surface_data.m_vertices.size();

		SDL_Log("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ vkDevice,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(surface_data.m_vertices.data());
		//staging_buffer.Unmap();

		SDL_Log("Vertices data copied to Staging Buffer.");

		SDL_Log("Creating Vertex Buffer.");

		surface_data.m_vertex_buffer = render::DeviceBuffer(vkDevice, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		SDL_Log("Copying Staging Buffer into Vertex Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), surface_data.m_vertex_buffer.GetBuffer(), buffer_size);

		SDL_Log("Destroying Staging Buffer.");
    }

    void SceneRenderer::CreateIndexBuffer(Mesh3DComponent::SurfaceData& surface_data)
    {
		if (surface_data.m_indices.empty())
			return;

		render::Renderer* render = render::Renderer::GetRenderer();
		render::RenderDevice* render_device = render->GetDevice();
		vk::Device vkDevice = render_device->Get_VkDevice();

		vk::DeviceSize buffer_size = sizeof(uint32_t) * surface_data.m_indices.size();

		SDL_Log("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ vkDevice,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(surface_data.m_indices.data());

		SDL_Log("Indices data copied to Staging Buffer.");

		SDL_Log("Creating Index Buffer.");

		surface_data.m_index_buffer = render::DeviceBuffer(vkDevice, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		SDL_Log("Copying Staging Buffer into Index Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), surface_data.m_index_buffer.GetBuffer(), buffer_size);

		SDL_Log("Destroying Staging Buffer.");
    }
}
