#include "Mesh3DComponent.h"

#include <Scene/Components/Transform3DComponent.h>

#include <Core/LogSystem.h>

namespace brr
{
    Mesh3DComponent::SurfaceData::SurfaceData(SurfaceData&& surface) noexcept
        : m_vertices(std::move(surface.m_vertices)),
          m_indices(std::move(surface.m_indices)),
          m_surfaceId(surface.m_surfaceId)
    {
    }

    Mesh3DComponent::SurfaceData& Mesh3DComponent::SurfaceData::operator=(SurfaceData&& surface) noexcept
    {
        m_vertices  = std::move(surface.m_vertices);
        m_indices   = std::move(surface.m_indices);
        m_surfaceId = surface.m_surfaceId;

        surface.m_surfaceId = render::SurfaceID::NULL_ID;

        return *this;
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3>&& vertices)
        : m_vertices(std::move(vertices))
    {
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3> vertices)
        : m_vertices(std::move(vertices))
    {
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3>&& vertices,
                                              std::vector<uint32_t>&& indices)
        : m_vertices(std::move(vertices)),
          m_indices(std::move(indices))
    {
    }

    Mesh3DComponent::SurfaceData::SurfaceData(std::vector<Vertex3> vertices,
                                              std::vector<uint32_t> indices)
        : m_vertices(std::move(vertices)),
          m_indices(std::move(indices))
    {
    }

    void Mesh3DComponent::RegisterGraphics()
    {
        vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_render_proxy && "Can't call 'Mesh3DComponent::RegisterGraphics' when SceneRenderer is NULL.");

        Transform3DComponent& owner_transform = GetEntity().GetComponent<Transform3DComponent>();
        for (auto& surface : m_surfaces)
        {
            surface.m_surfaceId = scene_render_proxy->CreateSurface(owner_transform, (void*)surface.GetVertices().data(),
                                                                    surface.GetVertices().size() * sizeof(Vertex3),
                                                                    (void*)surface.GetIndices().data(),
                                                                    surface.GetIndices().size() * sizeof(uint32_t));
        }
    }

    void Mesh3DComponent::UnregisterGraphics()
    {
        vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_render_proxy && "Can't call 'Mesh3DComponent::UnregisterGraphics' when SceneRenderer is NULL.");

        for (auto& surface : m_surfaces)
        {
            GetScene()->GetSceneRendererProxy()->DestroySurface(surface.GetRenderSurfaceID());
            surface.m_surfaceId = render::SurfaceID::NULL_ID;
        }
    }

    render::SurfaceID Mesh3DComponent::AddSurface(const std::vector<Vertex3>& vertices,
                                                  const std::vector<uint32_t>& indices)
    {
        BRR_LogInfo("Adding new Surface");
        SurfaceData& new_surface = m_surfaces.emplace_back(vertices, indices);
        if (!new_surface.GetVertices().empty())
        {
            if (vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy())
            {
                Transform3DComponent& owner_transform = GetEntity().GetComponent<Transform3DComponent>();

                new_surface.m_surfaceId = scene_render_proxy->CreateSurface(
                    owner_transform, (void*)new_surface.GetVertices().data(),
                    new_surface.GetVertices().size() * sizeof(Vertex3), (void*)new_surface.GetIndices().data(),
                    new_surface.GetIndices().size() * sizeof(uint32_t));
            }
        }
        return new_surface.m_surfaceId;
    }

    render::SurfaceID Mesh3DComponent::AddSurface(SurfaceData&& surface)
    {
        BRR_LogInfo("Adding new Surface");
        SurfaceData& new_surface = m_surfaces.emplace_back(std::forward<SurfaceData&&>(surface));
        if (!new_surface.GetVertices().empty())
        {
            if (vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy())
            {
                Transform3DComponent& owner_transform = GetEntity().GetComponent<Transform3DComponent>();
                new_surface.m_surfaceId               = new_surface.m_surfaceId = scene_render_proxy->CreateSurface(
                    owner_transform, (void*)new_surface.GetVertices().data(),
                    new_surface.GetVertices().size() * sizeof(Vertex3), (void*)new_surface.GetIndices().data(),
                    new_surface.GetIndices().size() * sizeof(uint32_t));
            }
        }
        return new_surface.m_surfaceId;
    }

    void Mesh3DComponent::RemoveSurface(render::SurfaceID surface_id)
    {
        BRR_LogInfo("Removing surface {}", uint32_t(surface_id));

        if (vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy())
        {
            scene_render_proxy->DestroySurface(surface_id);
        }
        //TODO: remove from surface vector
        //m_surfaces.erase(m_surfaces.begin() + surface_index);
    }
}
