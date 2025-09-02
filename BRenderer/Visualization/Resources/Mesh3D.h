#ifndef BRR_MESH3D_H
#define BRR_MESH3D_H

#include <vector>

#include <Core/Assets/Asset.h>
#include <Geometry/Geometry.h>
#include <Renderer/RenderingResourceIDs.h>
#include <Visualization/Resources/Material.h>

namespace brr::vis
{
    class Mesh3D : public Asset
    {
    public:

		Mesh3D() = default;

		Mesh3D(const Mesh3D& other);

		Mesh3D(Mesh3D&& other) noexcept;

		Mesh3D& operator=(const Mesh3D& other);

		Mesh3D& operator=(Mesh3D&& other) noexcept;

		~Mesh3D() override;

		render::SurfaceID AddSurface(const std::vector<Vertex3>& vertices, const std::vector<uint32_t>& indices, Ref<Material> material);

		void RemoveSurface(render::SurfaceID surface_id);

        [[nodiscard]] constexpr size_t GetSurfaceCount() const { return m_surfaces.size(); }

		std::vector<render::SurfaceID> GetSurfacesIDs() const;

		size_t GetSurfaceVertexCount(render::SurfaceID surface_id) const;

		size_t GetSurfaceIndexCount(render::SurfaceID surface_id) const;

		std::vector<Vertex3> GetSurfaceVertices(render::SurfaceID surface_id) const;

		std::vector<uint32_t> GetSurfaceIndices(render::SurfaceID surface_id) const;

    private:
        struct SurfaceData
		{
			SurfaceData() = default;

			SurfaceData(const SurfaceData& surface) = default;

			~SurfaceData() = default;
			
			SurfaceData& operator=(const SurfaceData& surface) = default;

			SurfaceData(SurfaceData&& surface) noexcept;

			SurfaceData& operator=(SurfaceData&& surface) noexcept;

			SurfaceData(std::vector<Vertex3>&& vertices);
			SurfaceData(std::vector<Vertex3> vertices);

			SurfaceData(std::vector<Vertex3>&& vertices, std::vector<uint32_t>&& indices);
			SurfaceData(std::vector<Vertex3> vertices, std::vector<uint32_t> indices);

			const std::vector<Vertex3>& GetVertices() const { return m_vertices; }
			const std::vector<uint32_t>& GetIndices() const { return m_indices; }

			render::SurfaceID GetRenderSurfaceID() const { return m_surface_id; }

			std::vector<Vertex3>  m_vertices{};
			std::vector<uint32_t> m_indices{};

			render::SurfaceID m_surface_id = render::SurfaceID();
            Ref<Material> m_material;
		};

		std::vector<SurfaceData> m_surfaces;
    };
}

#endif