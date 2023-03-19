#ifndef BRR_RENDERSURFACE_H
#define BRR_RENDERSURFACE_H
#include "DeviceBuffer.h"
#include "Geometry/Geometry.h"


namespace brr::render
{
	class RenderDevice;

	struct RenderSurface
	{

		std::unique_ptr<DeviceBuffer> vertex_buffer;
		std::unique_ptr<DeviceBuffer> index_buffer;

		uint32_t vertex_count = 0;
		uint32_t index_count = 0;

		RenderSurface(RenderDevice* device, std::vector<Vertex3_PosColor>& vertices, std::vector<uint32_t>& indices);

		void Bind(vk::CommandBuffer command_buffer);
		void Draw(vk::CommandBuffer command_buffer);

		void CreateVertexBuffer(RenderDevice* device, std::vector<Vertex3_PosColor>& vertices);
		void CreateIndexBuffer(RenderDevice* device, std::vector<uint32_t>& indices);
	};

}

#endif