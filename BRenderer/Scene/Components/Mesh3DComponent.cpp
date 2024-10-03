#include "Mesh3DComponent.h"

#include <Core/LogSystem.h>
#include <Scene/Components/Transform3DComponent.h>

namespace brr
{
    void Mesh3DComponent::SetMesh(const Ref<vis::Mesh3D>& mesh)
    {
        if (mesh == m_mesh)
        {
            return;
        }

        if (GetScene()->GetSceneRendererProxy())
        {
            UnregisterGraphics();
        }

        m_mesh = mesh;

        if (GetScene()->GetSceneRendererProxy())
        {
            RegisterGraphics();
        }
    }

    void Mesh3DComponent::RegisterGraphics()
    {
        if (!m_mesh)
            return;

        vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_render_proxy && "Can't call 'Mesh3DComponent::RegisterGraphics' when SceneRenderer is NULL.");

        Transform3DComponent& owner_transform = GetEntity().GetComponent<Transform3DComponent>();
        std::vector<render::SurfaceID> surface_ids = m_mesh->GetSurfacesIDs();
        for (auto& surface_id : surface_ids)
        {
            scene_render_proxy->AppendSurfaceToEntity(owner_transform, surface_id);
        }
    }

    void Mesh3DComponent::UnregisterGraphics()
    {
        if (!m_mesh)
            return;

        vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_render_proxy && "Can't call 'Mesh3DComponent::UnregisterGraphics' when SceneRenderer is NULL.");

        std::vector<render::SurfaceID> surface_ids = m_mesh->GetSurfacesIDs();
        for (auto& surface_id : surface_ids)
        {
            scene_render_proxy->EraseSurfaceFromEntity(surface_id);
        }
    }
}
