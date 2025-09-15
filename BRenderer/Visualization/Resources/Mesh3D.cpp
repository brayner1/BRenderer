#include "Mesh3D.h"

#include <Renderer/RenderThread.h>
#include <Visualization/SceneRendererProxy.h>

using namespace brr;
using namespace brr::vis;

Mesh3D::Mesh3D(const Mesh3D& other)
{
    *this = other;
}

Mesh3D::Mesh3D(Mesh3D&& other) noexcept
{
    *this = std::move(other);
}

Mesh3D& Mesh3D::operator=(const Mesh3D& other)
{
    m_surfaces = other.m_surfaces;
    return *this;
}

Mesh3D& Mesh3D::operator=(Mesh3D&& other)
    noexcept
{
    m_surfaces = std::move(other.m_surfaces);
    other.m_surfaces.clear();
    return *this;
}

Mesh3D::~Mesh3D()
{
    for (auto& surface : m_surfaces)
    {
        if (surface.m_surface_id.IsValid())
            render::RenderThread::ResourceCmd_DestroySurface(surface.m_surface_id);
    }
}

render::SurfaceID Mesh3D::AddSurface(const std::vector<Vertex3>& vertices,
                                     const std::vector<uint32_t>& indices,
                                     Ref<Material> material)
{
    BRR_LogInfo("Adding new Surface");
    SurfaceData& new_surface = m_surfaces.emplace_back(vertices, indices);
    if (!new_surface.GetVertices().empty())
    {
        render::MaterialID material_id = material ? material->GetMaterialID() : render::MaterialID();
        new_surface.m_material = material;
        new_surface.m_surface_id = render::RenderThread::ResourceCmd_CreateSurface((void*)new_surface.GetVertices().data(),
            new_surface.GetVertices().size() * sizeof(
                Vertex3),
            (void*)new_surface.GetIndices().data(),
            new_surface.GetIndices().size() * sizeof(
                uint32_t), material_id);
    }
    return new_surface.m_surface_id;
}

void Mesh3D::RemoveSurface(render::SurfaceID surface_id)
{
    BRR_LogInfo("Removing surface {}", uint64_t(surface_id));
    size_t removed_count = std::erase_if(m_surfaces, [surface_id](SurfaceData& surface_data) { return surface_data.m_surface_id == surface_id; });

    if (removed_count > 0)
    {
        render::RenderThread::ResourceCmd_DestroySurface(surface_id);
    }
}

std::vector<render::SurfaceID> Mesh3D::GetSurfacesIDs() const
{
    std::vector<render::SurfaceID> surface_ids;
    for (const SurfaceData& surface : m_surfaces)
    {
        surface_ids.push_back(surface.m_surface_id);
    }
    return surface_ids;
}

Ref<Material> Mesh3D::GetSurfaceMaterial(render::SurfaceID surface_id) const
{
    auto iter = std::find_if(m_surfaces.begin(), m_surfaces.end(), [surface_id](const SurfaceData& surface) { return surface.m_surface_id == surface_id; });
    if (iter != m_surfaces.end())
    {
        return iter->m_material;
    }
    return Ref<Material>();
}

size_t Mesh3D::GetSurfaceVertexCount(render::SurfaceID surface_id) const
{
    auto iter = std::find_if(m_surfaces.begin(), m_surfaces.end(), [surface_id](const SurfaceData& surface) { return surface.m_surface_id == surface_id; });
    if (iter != m_surfaces.end())
    {
        return iter->m_vertices.size();
    }
    return 0;
}

size_t Mesh3D::GetSurfaceIndexCount(render::SurfaceID surface_id) const
{
    auto iter = std::find_if(m_surfaces.begin(), m_surfaces.end(), [surface_id](const SurfaceData& surface) { return surface.m_surface_id == surface_id; });
    if (iter != m_surfaces.end())
    {
        return iter->m_indices.size();
    }
    return 0;
}

std::vector<Vertex3> Mesh3D::GetSurfaceVertices(render::SurfaceID surface_id) const
{
    auto iter = std::find_if(m_surfaces.begin(), m_surfaces.end(), [surface_id](const SurfaceData& surface) { return surface.m_surface_id == surface_id; });
    if (iter != m_surfaces.end())
    {
        return iter->m_vertices;
    }
    return {};
}

std::vector<uint32_t> Mesh3D::GetSurfaceIndices(render::SurfaceID surface_id) const
{
    auto iter = std::find_if(m_surfaces.begin(), m_surfaces.end(), [surface_id](const SurfaceData& surface) { return surface.m_surface_id == surface_id; });
    if (iter != m_surfaces.end())
    {
        return iter->m_indices;
    }
    return {};
}

Mesh3D::SurfaceData::SurfaceData(SurfaceData&& surface) noexcept
    : m_vertices(std::move(surface.m_vertices)),
      m_indices(std::move(surface.m_indices)),
      m_surface_id(surface.m_surface_id)
{
}

Mesh3D::SurfaceData& Mesh3D::SurfaceData::operator=(SurfaceData&& surface) noexcept
{
    m_vertices   = std::move(surface.m_vertices);
    m_indices    = std::move(surface.m_indices);
    m_surface_id = surface.m_surface_id;

    surface.m_surface_id = render::SurfaceID();

    return *this;
}

Mesh3D::SurfaceData::SurfaceData(std::vector<Vertex3>&& vertices)
    : m_vertices(std::move(vertices))
{
}

Mesh3D::SurfaceData::SurfaceData(std::vector<Vertex3> vertices)
    : m_vertices(std::move(vertices))
{
}

Mesh3D::SurfaceData::SurfaceData(std::vector<Vertex3>&& vertices,
                                 std::vector<uint32_t>&& indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices))
{
}

Mesh3D::SurfaceData::SurfaceData(std::vector<Vertex3> vertices,
                                 std::vector<uint32_t> indices)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices))
{
}
