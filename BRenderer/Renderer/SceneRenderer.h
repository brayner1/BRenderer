#ifndef BRR_SCENERENDERER_H
#define BRR_SCENERENDERER_H

#include "Scene/Components/Mesh3DComponent.h"

namespace brr::render
{

    class SceneRenderer
    {
    public:

        SceneRenderer(entt::registry& registry);

    private:

        void OnAddedMesh3D(entt::registry& registry, entt::entity entity);

        void CreateVertexBuffer(Mesh3DComponent::SurfaceData& surface_data);
        void CreateIndexBuffer(Mesh3DComponent::SurfaceData& surface_data);


        entt::registry* m_registry = nullptr;
    };

}

#endif
