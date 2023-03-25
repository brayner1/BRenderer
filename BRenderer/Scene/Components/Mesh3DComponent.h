#ifndef BRR_MESH3DCOMPONENT_H
#define BRR_MESH3DCOMPONENT_H
#include "Geometry/Geometry.h"
#include "Renderer/DeviceBuffer.h"

namespace brr
{

	struct Mesh3DUniform
	{
		glm::mat4 model_matrix;
	};

	struct Mesh3DComponent
	{
		struct SurfaceData
		{
			SurfaceData();

			SurfaceData(const SurfaceData& surface);

			SurfaceData(std::vector<Vertex3_PosColor>&& vertices);
			SurfaceData(std::vector<Vertex3_PosColor> vertices);

			SurfaceData(std::vector<Vertex3_PosColor>&& vertices, std::vector<uint32_t>&& indices);
			SurfaceData(std::vector<Vertex3_PosColor> vertices, std::vector<uint32_t> indices);

			void Bind(vk::CommandBuffer command_buffer) const;
			void Draw(vk::CommandBuffer command_buffer) const;

		private:
			void CreateVertexBuffer();
			void CreateIndexBuffer();

			std::vector<Vertex3_PosColor> m_vertices{};
			std::vector<uint32_t>		  m_indices{};

			render::DeviceBuffer m_vertex_buffer;
			render::DeviceBuffer m_index_buffer;

			render::DeviceBuffer m_uniform_buffer;

			vk::DescriptorSet m_descriptor_set;
		};

		std::vector<SurfaceData> surfaces{};
	};
}

#endif