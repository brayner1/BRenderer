#include "Renderer/Geometry/Mesh2D.h"
#include "Renderer/Geometry/Geometry.h"
#include "Renderer/Renderer.h"

namespace brr::render
{
	Mesh2D::Mesh2D(vk::Device device, std::vector<Vertex2_PosColor>&& vertices, std::vector<uint32_t>&& indices):
		device_(device), vertices_vec_(std::move(vertices)), indices_vec_(std::move(indices))
	{
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	Mesh2D::Mesh2D(vk::Device device, const std::vector<Vertex2_PosColor>& vertices,
		const std::vector<uint32_t>& indices):
		device_(device), vertices_vec_(vertices), indices_vec_(indices)
	{
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	Mesh2D::Mesh2D(Mesh2D&& mesh) noexcept:
		device_(mesh.device_),
		vertices_vec_(std::move(mesh.vertices_vec_)), indices_vec_(std::move(mesh.indices_vec_)),
		vertex_buffer_(std::move(mesh.vertex_buffer_)),
		index_buffer(std::move(mesh.index_buffer)), usesIndexBuffer(mesh.usesIndexBuffer)
	{
		mesh.usesIndexBuffer = false;
	}

	Mesh2D::Mesh2D(const Mesh2D& mesh) :
		device_(mesh.device_), vertices_vec_(mesh.vertices_vec_), indices_vec_(mesh.indices_vec_)
	{
		CreateVertexBuffer();
		CreateIndexBuffer();
	}

	Mesh2D& Mesh2D::operator=(Mesh2D&& mesh) noexcept
	{
		device_ = mesh.device_;

		vertices_vec_ = std::move(mesh.vertices_vec_);
		vertex_buffer_ = std::move(mesh.vertex_buffer_);

		indices_vec_ = std::move(mesh.indices_vec_);
		usesIndexBuffer = mesh.usesIndexBuffer;
		index_buffer = std::move(mesh.index_buffer);
		mesh.usesIndexBuffer = false;

		return *this;
	}

	Mesh2D& Mesh2D::operator=(const Mesh2D& mesh)
	{
		device_ = mesh.device_;

		vertices_vec_ = mesh.vertices_vec_;

		indices_vec_ = mesh.indices_vec_;
		usesIndexBuffer = mesh.usesIndexBuffer;

		CreateVertexBuffer();
		CreateIndexBuffer();

		return *this;
	}

	Mesh2D::~Mesh2D()
	{
		SDL_Log("Mesh destroyed");
	}

	void Mesh2D::Bind(vk::CommandBuffer command_buffer)
	{
		command_buffer.bindVertexBuffers(0, vertex_buffer_->GetBuffer(), {0});

		if (usesIndexBuffer)
		{
			command_buffer.bindIndexBuffer(index_buffer->GetBuffer(), 0, vk::IndexType::eUint32);
		}
	}

	void Mesh2D::Draw(vk::CommandBuffer command_buffer)
	{
		if (usesIndexBuffer)
		{
			command_buffer.drawIndexed(indices_vec_.size(), 1, 0, 0, 0);
		}
		else
		{
			command_buffer.draw(vertices_vec_.size(), 1, 0, 0);
		}
	}

	void Mesh2D::CreateVertexBuffer()
	{
		assert(!vertices_vec_.empty() && "Vertices data can't be empty.");
		Renderer* render = Renderer::GetRenderer();

		vk::DeviceSize buffer_size = sizeof(Vertex2_PosColor) * vertices_vec_.size();

		SDL_Log("Creating Staging Buffer.");

		DeviceBuffer staging_buffer {device_,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(vertices_vec_.data());
		//staging_buffer.Unmap();

		SDL_Log("Vertices data copied to Staging Buffer.");

		SDL_Log("Creating Vertex Buffer.");

		vertex_buffer_ = std::make_unique<DeviceBuffer>(device_, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		SDL_Log("Copying Staging Buffer into Vertex Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), vertex_buffer_->GetBuffer(), buffer_size);

		SDL_Log("Destroying Staging Buffer.");
	}

	void Mesh2D::CreateIndexBuffer()
	{
		if (indices_vec_.empty())
			return;

		usesIndexBuffer = true;
		Renderer* render = Renderer::GetRenderer();

		vk::DeviceSize buffer_size = sizeof(Vertex2_PosColor) * indices_vec_.size();

		SDL_Log("Creating Staging Buffer.");

		DeviceBuffer staging_buffer{ device_,
			buffer_size,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent };

		staging_buffer.Map();
		staging_buffer.WriteToBuffer(indices_vec_.data());

		SDL_Log("Indices data copied to Staging Buffer.");

		SDL_Log("Creating Index Buffer.");

		index_buffer = std::make_unique<DeviceBuffer>(device_, buffer_size,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		SDL_Log("Copying Staging Buffer into Index Buffer.");

		render->Copy_Buffer(staging_buffer.GetBuffer(), index_buffer->GetBuffer(), buffer_size);

		SDL_Log("Destroying Staging Buffer.");
	}
}
