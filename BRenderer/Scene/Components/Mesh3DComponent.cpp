#include "Scene/Components/Mesh3DComponent.h"

#include "Renderer/Renderer.h"

namespace brr
{
	Mesh3DComponent::SurfaceData::SurfaceData()
	{
	}

	Mesh3DComponent::SurfaceData::SurfaceData(const SurfaceData& surface)
	{
		m_vertices = surface.m_vertices;
		m_indices = surface.m_indices;

		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor>&& vertices)
	: m_vertices(std::move(vertices))
	{
		CreateVertexBuffer();
	}

	Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor> vertices)
	: m_vertices(std::move(vertices))
	{
		CreateVertexBuffer();
	}

	Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor>&& vertices, std::vector<uint32_t>&& indices)
	: m_vertices(std::move(vertices)), m_indices(std::move(indices))
	{
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor> vertices, std::vector<uint32_t> indices)
	: m_vertices(std::move(vertices)), m_indices(std::move(indices))
	{
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	void Mesh3DComponent::SurfaceData::Bind(vk::CommandBuffer command_buffer) const
	{
		assert(m_vertex_buffer.IsValid() && "Vertex buffer must be valid to bind to a command buffer.");
		command_buffer.bindVertexBuffers(0, m_vertex_buffer.GetBuffer(), { 0 });

		if (m_index_buffer.IsValid())
		{
			command_buffer.bindIndexBuffer(m_index_buffer.GetBuffer(), 0, vk::IndexType::eUint32);
		}

		//command_buffer.bindDescriptorSets(1, );
	}

	void Mesh3DComponent::SurfaceData::Draw(vk::CommandBuffer command_buffer) const
	{
		assert(m_vertex_buffer.IsValid() && "Vertex buffer must be valid to Draw to a command buffer.");
		if (m_index_buffer.IsValid())
		{
			command_buffer.drawIndexed(m_indices.size(), 1, 0, 0, 0);
		}
		else
		{
			command_buffer.draw(m_vertices.size(), 1, 0, 0);
		}
	}

	void Mesh3DComponent::SurfaceData::CreateVertexBuffer()
	{
		assert(!m_vertices.empty() && "Vertices data can't be empty.");
		render::Renderer* render = render::Renderer::GetRenderer();
		render::RenderDevice* render_device = render->GetDevice();
		vk::Device vkDevice = render_device->Get_VkDevice();

		vk::DeviceSize buffer_size = sizeof(Vertex3_PosColor) * m_vertices.size();

		SDL_Log("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ vkDevice,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(m_vertices.data());
		//staging_buffer.Unmap();

		SDL_Log("Vertices data copied to Staging Buffer.");

		SDL_Log("Creating Vertex Buffer.");

		m_vertex_buffer = render::DeviceBuffer(vkDevice, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		SDL_Log("Copying Staging Buffer into Vertex Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), m_vertex_buffer.GetBuffer(), buffer_size);

		SDL_Log("Destroying Staging Buffer.");
	}

	void Mesh3DComponent::SurfaceData::CreateIndexBuffer()
	{
		if (m_indices.empty())
			return;

		render::Renderer* render = render::Renderer::GetRenderer();
		render::RenderDevice* render_device = render->GetDevice();
		vk::Device vkDevice = render_device->Get_VkDevice();

		vk::DeviceSize buffer_size = sizeof(uint32_t) * m_indices.size();

		SDL_Log("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ vkDevice,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(m_indices.data());

		SDL_Log("Indices data copied to Staging Buffer.");

		SDL_Log("Creating Index Buffer.");

		m_index_buffer = render::DeviceBuffer(vkDevice, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		SDL_Log("Copying Staging Buffer into Index Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), m_index_buffer.GetBuffer(), buffer_size);

		SDL_Log("Destroying Staging Buffer.");
	}
}
