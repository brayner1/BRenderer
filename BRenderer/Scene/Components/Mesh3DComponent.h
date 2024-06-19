#ifndef BRR_MESH3DCOMPONENT_H
#define BRR_MESH3DCOMPONENT_H

#include <Scene/Components/EntityComponent.h>
#include <Geometry/Geometry.h>

namespace brr
{
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

			SurfaceData(std::vector<Vertex3>&& vertices);
			SurfaceData(std::vector<Vertex3> vertices);

			SurfaceData(std::vector<Vertex3>&& vertices, std::vector<uint32_t>&& indices);
			SurfaceData(std::vector<Vertex3> vertices, std::vector<uint32_t> indices);

			const std::vector<Vertex3>& GetVertices() const { return m_vertices; }
			const std::vector<uint32_t>& GetIndices() const { return m_indices; }

			render::SurfaceID GetRenderSurfaceID() const { return m_surfaceId; }

		private:
			friend class Mesh3DComponent;

			std::vector<Vertex3>  m_vertices{};
			std::vector<uint32_t> m_indices{};

			render::SurfaceID m_surfaceId = render::SurfaceID::NULL_ID;
		};

		render::SurfaceID AddSurface(const std::vector<Vertex3>& vertices, const std::vector<uint32_t>& indices);
		render::SurfaceID AddSurface(SurfaceData&& surface);

		void RemoveSurface(render::SurfaceID surface_id);

		const std::vector<SurfaceData>& GetMeshSurfaces() const { return m_surfaces; }

        [[nodiscard]] constexpr size_t GetSurfaceCount() const { return m_surfaces.size(); }

	public:

		void RegisterGraphics();
		void UnregisterGraphics();

    private:

		std::vector<SurfaceData> m_surfaces;
	};
}

#endif