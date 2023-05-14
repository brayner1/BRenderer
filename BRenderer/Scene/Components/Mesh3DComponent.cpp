#include "Scene/Components/Mesh3DComponent.h"

namespace brr
{
    Mesh3DComponent::SurfaceData::SurfaceData()
    : m_surfaceId(currentSurfaceID++)
    {

    }

    Mesh3DComponent::SurfaceData::SurfaceData(const SurfaceData& surface)
    : m_vertices(surface.m_vertices),
      m_indices(surface.m_indices),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor>&& vertices)
    : m_vertices(std::move(vertices)),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor> vertices)
    : m_vertices(std::move(vertices)),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor>&& vertices, std::vector<uint32_t>&& indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices)),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3_PosColor> vertices, std::vector<uint32_t> indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices)),
      m_surfaceId(currentSurfaceID++)
    {
        m_need_update = true;
    }

    Mesh3DComponent::Mesh3DComponent(std::vector<SurfaceData>&& surfaces)
    : m_surfaces(surfaces)
    {}

}
