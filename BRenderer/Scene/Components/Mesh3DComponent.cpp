#include "Scene/Components/Mesh3DComponent.h"

#include <Renderer/SceneRenderer.h>

namespace brr
{
    Mesh3DComponent::SurfaceData::SurfaceData()
    {}

    Mesh3DComponent::SurfaceData::SurfaceData(const SurfaceData& surface)
    : m_vertices(surface.m_vertices),
      m_indices(surface.m_indices)
    {}

    Mesh3DComponent::SurfaceData::SurfaceData(SurfaceData&& surface) noexcept
    : m_vertices(std::move(surface.m_vertices)),
      m_indices(std::move(surface.m_indices)),
      m_surfaceId(surface.m_surfaceId)
    {}

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
        m_dirty_surfaces.insert(m_surfaces.size());
        SurfaceData& new_surface = m_surfaces.emplace_back(vertices, indices);
        if (!new_surface.GetVertices().empty()
         && !GetEntity().HasAllComponents<render::SceneRenderer::MeshDirty>())
        {
            GetEntity().AddComponent<render::SceneRenderer::MeshDirty>();
        }
        return m_surfaces.size() - 1;
    }

    uint32_t Mesh3DComponent::AddSurface(SurfaceData&& surface)
    {
        BRR_LogInfo("Adding new Surface");
        m_dirty_surfaces.insert(m_surfaces.size());
        SurfaceData& new_surface = m_surfaces.emplace_back(std::forward<SurfaceData&&>(surface));
        if (!new_surface.GetVertices().empty()
         && !GetEntity().HasAllComponents<render::SceneRenderer::MeshDirty>())
        {
            GetEntity().AddComponent<render::SceneRenderer::MeshDirty>();
        }
        return m_surfaces.size() - 1;
    }
}
