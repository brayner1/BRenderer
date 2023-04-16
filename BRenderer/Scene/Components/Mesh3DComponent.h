#ifndef BRR_MESH3DCOMPONENT_H
#define BRR_MESH3DCOMPONENT_H
#include "Geometry/Geometry.h"
#include "Renderer/DeviceBuffer.h"

namespace brr
{
    namespace render
    {
        class SceneRenderer;
    }

    struct Mesh3DUniform
	{
		glm::mat4 model_matrix;
	};

	struct Mesh3DComponent
	{
		struct SurfaceData
		{
            inline static uint64_t currentSurfaceID = 0;

			SurfaceData();

			SurfaceData(const SurfaceData& surface);

			SurfaceData(std::vector<Vertex3_PosColor>&& vertices);
			SurfaceData(std::vector<Vertex3_PosColor> vertices);

			SurfaceData(std::vector<Vertex3_PosColor>&& vertices, std::vector<uint32_t>&& indices);
			SurfaceData(std::vector<Vertex3_PosColor> vertices, std::vector<uint32_t> indices);

			[[nodiscard]] bool NeedUpdate() const { return m_need_update; }

			[[nodiscard]] uint64_t GetSurfaceID() const { return m_surfaceId; }

		private:
			friend class brr::render::SceneRenderer;

			std::vector<Vertex3_PosColor> m_vertices{};
			std::vector<uint32_t>		  m_indices{};

			uint64_t m_surfaceId;

			bool m_need_update = false;
		};

		std::vector<SurfaceData> surfaces{};
	};
}

#endif