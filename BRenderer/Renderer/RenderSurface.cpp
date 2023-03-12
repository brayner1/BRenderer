#include "Renderer/RenderSurface.h"
#include "Renderer/Renderer.h"

namespace brr::render
{

	RenderSurface::RenderSurface(RenderDevice* device, std::vector<Vertex3_PosColor>& vertices, std::vector<uint32_t>& indices)
	{
		CreateVertexBuffer(device, vertices);
		if (indices.size() > 0)
		{
			CreateIndexBuffer(device, indices);
		}
	}

	void RenderSurface::Bind(vk::CommandBuffer command_buffer)
	{
		command_buffer.bindVertexBuffers(0, vertex_buffer->GetBuffer(), { 0 });

		if (index_buffer->IsValid())
		{
			command_buffer.bindIndexBuffer(index_buffer->GetBuffer(), 0, vk::IndexType::eUint32);
		}
	}

	void RenderSurface::Draw(vk::CommandBuffer command_buffer)
	{
		if (index_buffer->IsValid())
		{
			command_buffer.drawIndexed(index_count, 1, 0, 0, 0);
		}
		else
		{
			command_buffer.draw(vertex_count, 1, 0, 0);
		}
	}

	void RenderSurface::CreateVertexBuffer(RenderDevice* device, std::vector<Vertex3_PosColor>& vertices)
	{
		assert(!vertices.empty() && "Vertices data can't be empty.");
		render::Renderer* render = render::Renderer::GetRenderer();

		vk::DeviceSize buffer_size = sizeof(Vertex2_PosColor) * vertices.size();

		SDL_Log("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ device->Get_VkDevice(),
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(vertices.data());
		//staging_buffer.Unmap();

		SDL_Log("Vertices data copied to Staging Buffer.");

		SDL_Log("Creating Vertex Buffer.");

		vertex_buffer = std::make_unique<render::DeviceBuffer>( device->Get_VkDevice(), buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		SDL_Log("Copying Staging Buffer into Vertex Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), vertex_buffer->GetBuffer(), buffer_size);

		SDL_Log("Destroying Staging Buffer.");
	}

	void RenderSurface::CreateIndexBuffer(RenderDevice* device, std::vector<uint32_t>& indices)
	{
		assert(indices.size() > 0 && "Can't create IndexBuffer without indices!");

		render::Renderer* render = render::Renderer::GetRenderer();

		vk::DeviceSize buffer_size = sizeof(Vertex2_PosColor) * indices.size();

		SDL_Log("Creating Staging Buffer.");

		render::DeviceBuffer staging_buffer{ device->Get_VkDevice(),
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(indices.data());

		SDL_Log("Indices data copied to Staging Buffer.");

		SDL_Log("Creating Index Buffer.");

		index_buffer = std::make_unique<render::DeviceBuffer>(device->Get_VkDevice(), buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		SDL_Log("Copying Staging Buffer into Index Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), index_buffer->GetBuffer(), buffer_size);

		SDL_Log("Destroying Staging Buffer.");
	}

}
