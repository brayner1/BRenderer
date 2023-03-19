#ifndef BRR_MESH2D_H
#define BRR_MESH2D_H
#include "Geometry/Geometry.h"
#include "Renderer/DeviceBuffer.h"

namespace brr
{
	class Mesh2D
	{
	public:
		Mesh2D(vk::Device device) : device_(device) {}
		Mesh2D(vk::Device device, std::vector<Vertex2_PosColor>&& vertices) : device_(device), vertices_vec_(std::move(vertices))
		{
			CreateVertexBuffer();
		}

		Mesh2D(vk::Device device, std::vector<Vertex2_PosColor>&& vertices, std::vector<uint32_t>&& indices);
		Mesh2D(vk::Device device, const std::vector<Vertex2_PosColor>& vertices, const std::vector<uint32_t>& indices);

		Mesh2D(Mesh2D&& mesh) noexcept;
		Mesh2D(const Mesh2D& mesh);

		Mesh2D& operator=(Mesh2D&& mesh) noexcept;
		Mesh2D& operator=(const Mesh2D& mesh);

		~Mesh2D();

		void Bind(vk::CommandBuffer command_buffer);
		void Draw(vk::CommandBuffer command_buffer);

	private:

		void CreateVertexBuffer();
		void CreateIndexBuffer();

		vk::Device device_ {};

		std::unique_ptr<render::DeviceBuffer> vertex_buffer_ {};

		std::unique_ptr<render::DeviceBuffer> index_buffer {};

		std::vector<Vertex2_PosColor> vertices_vec_ {};
		std::vector<uint32_t> indices_vec_ {};
		bool usesIndexBuffer = false;
	};
}

#endif