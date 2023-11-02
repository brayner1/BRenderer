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
			SurfaceData() = default;

			SurfaceData(const SurfaceData& surface) = default;
			
			SurfaceData& operator=(const SurfaceData& surface) = default;

			SurfaceData(SurfaceData&& surface) noexcept;

			SurfaceData& operator=(SurfaceData&& surface) noexcept;

			SurfaceData(std::vector<Vertex3_PosColor>&& vertices);
			SurfaceData(std::vector<Vertex3_PosColor> vertices);

			SurfaceData(std::vector<Vertex3_PosColor>&& vertices, std::vector<uint32_t>&& indices);
			SurfaceData(std::vector<Vertex3_PosColor> vertices, std::vector<uint32_t> indices);

			const std::vector<Vertex3_PosColor>& GetVertices() const { return m_vertices; }
			const std::vector<uint32_t>& GetIndices() const { return m_indices; }

			uint64_t GetRenderSurfaceID() const { return m_surfaceId; }

		private:
			friend class Mesh3DComponent;

			std::vector<Vertex3_PosColor> m_vertices{};
			std::vector<uint32_t>		  m_indices{};

			uint64_t m_surfaceId = -1;
		};

		uint32_t AddSurface(const std::vector<Vertex3_PosColor>& vertices, const std::vector<uint32_t>& indices);
		uint32_t AddSurface(SurfaceData&& surface);

		void RemoveSurface(uint32_t surface_index);

        [[nodiscard]] constexpr size_t GetSurfaceCount() const { return m_surfaces.size(); }


    private:

		std::vector<SurfaceData> m_surfaces;
	};
}

#endif