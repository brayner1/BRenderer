#ifndef BRR_MESH3DCOMPONENT_H
#define BRR_MESH3DCOMPONENT_H

#include <Scene/Components/EntityComponent.h>
#include <Geometry/Geometry.h>

namespace brr
{
    namespace render
    {
        class SceneRenderer;
    }

	class Mesh3DComponent : public EntityComponent
	{
    public:
		struct SurfaceData
		{
			SurfaceData();

			SurfaceData(const SurfaceData& surface);

			SurfaceData(SurfaceData&& surface) noexcept;

			SurfaceData(std::vector<Vertex3_PosColor>&& vertices);
			SurfaceData(std::vector<Vertex3_PosColor> vertices);

			SurfaceData(std::vector<Vertex3_PosColor>&& vertices, std::vector<uint32_t>&& indices);
			SurfaceData(std::vector<Vertex3_PosColor> vertices, std::vector<uint32_t> indices);

			const std::vector<Vertex3_PosColor>& GetVertices() const { return m_vertices; }
			const std::vector<uint32_t>& GetIndices() const { return m_indices; }

			void SetRenderSurfaceID(uint64_t surface_id) { m_surfaceId = surface_id; }
			uint64_t GetRenderSurfaceID() const { return m_surfaceId; }

			[[nodiscard]] bool isDirty() const { return m_isDirty; }

			void SetIsDirty(bool isDirty) { m_isDirty = isDirty; }

		private:

			std::vector<Vertex3_PosColor> m_vertices{};
			std::vector<uint32_t>		  m_indices{};

			uint64_t m_surfaceId = -1;
			bool m_isDirty = true;
		};

		uint32_t AddSurface(const std::vector<Vertex3_PosColor>& vertices, const std::vector<uint32_t>& indices);
		uint32_t AddSurface(SurfaceData&& surface);

        [[nodiscard]] constexpr size_t GetSurfaceCount() const { return m_surfaces.size(); }


    private:
		friend class brr::render::SceneRenderer;

		std::vector<SurfaceData> m_surfaces{};
		std::set<uint32_t> m_dirty_surfaces{};
	};
}

#endif