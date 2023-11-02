#include "Mesh3DComponent.h"

#include <Visualization/SceneRenderer.h>

namespace brr
{
    Mesh3DComponent::SurfaceData::SurfaceData(SurfaceData&& surface) noexcept
    : m_vertices(std::move(surface.m_vertices)),
      m_indices(std::move(surface.m_indices)),
      m_surfaceId(surface.m_surfaceId)
    {}

    Mesh3DComponent::SurfaceData& Mesh3DComponent::SurfaceData::operator=(SurfaceData&& surface) noexcept
    {
        m_vertices = std::move(surface.m_vertices);
        m_indices = std::move(surface.m_indices);
        m_surfaceId = surface.m_surfaceId;

        surface.m_surfaceId = -1;

        return *this;
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor>&& vertices)
    : m_vertices(std::move(vertices))
    {}

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor> vertices)
    : m_vertices(std::move(vertices))
    {}

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor>&& vertices, std::vector<uint32_t>&& indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices))
    {}

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor> vertices, std::vector<uint32_t> indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices))
    {}

    uint32_t Mesh3DComponent::AddSurface(const std::vector<Vertex3_PosColor>& vertices,
                                         const std::vector<uint32_t>& indices)
    {
        BRR_LogInfo("Adding new Surface");
        SurfaceData& new_surface = m_surfaces.emplace_back(vertices, indices);
        if (!new_surface.GetVertices().empty())
        {
            new_surface.m_surfaceId = static_cast<uint64_t>(GetScene()->GetSceneRenderer()->CreateNewSurface(
                new_surface, GetEntity()));
        }
        return m_surfaces.size() - 1;
    }

    uint32_t Mesh3DComponent::AddSurface(SurfaceData&& surface)
    {
        BRR_LogInfo("Adding new Surface");
        SurfaceData& new_surface = m_surfaces.emplace_back(std::forward<SurfaceData&&>(surface));
        if (!new_surface.GetVertices().empty())
        {
            new_surface.m_surfaceId = static_cast<uint64_t>(GetScene()->GetSceneRenderer()->CreateNewSurface(
                new_surface, GetEntity()));
        }
        return m_surfaces.size() - 1;
    }

    void Mesh3DComponent::RemoveSurface(uint32_t surface_index)
    {
        assert((surface_index > 0 && surface_index < m_surfaces.size() - 1) && "Invalid surface index");
        BRR_LogInfo("Removing surface {}", surface_index);
        GetScene()->GetSceneRenderer()->RemoveSurface(
            static_cast<vis::SurfaceId>(m_surfaces[surface_index].GetRenderSurfaceID()));
        m_surfaces.erase(m_surfaces.begin() + surface_index);
    }
}
