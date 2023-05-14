#ifndef BRR_MESH3DCOMPONENT_H
#define BRR_MESH3DCOMPONENT_H
#include "Geometry/Geometry.h"

namespace brr
{
    namespace render
    {
        class SceneRenderer;
    }

	class Mesh3DComponent
	{
    public:
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
			void SetUpdated() { m_need_update = false; }

			[[nodiscard]] uint64_t GetSurfaceID() const { return m_surfaceId; }

		private:
			friend class brr::render::SceneRenderer;

			std::vector<Vertex3_PosColor> m_vertices{};
			std::vector<uint32_t>		  m_indices{};

			uint64_t m_surfaceId;

			bool m_need_update = false;
		};

        explicit Mesh3DComponent(std::vector<SurfaceData>&& surfaces);

		std::vector<SurfaceData>::iterator GetSurfacesBegin() { return m_surfaces.begin(); }
		std::vector<SurfaceData>::iterator GetSurfacesEnd() { return m_surfaces.end(); }

		std::vector<SurfaceData>::iterator begin() { return GetSurfacesBegin(); }
		std::vector<SurfaceData>::iterator end() { return GetSurfacesEnd(); }


    private:
		std::vector<SurfaceData> m_surfaces{};
	};
}

#endif