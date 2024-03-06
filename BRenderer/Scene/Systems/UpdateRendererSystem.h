#ifndef BRR_UPDATERENDERERSYSTEM_H
#define BRR_UPDATERENDERERSYSTEM_H

namespace brr
{
    namespace vis
    {
        class SceneRenderer;
    }

    class Mesh3DRendererComponent;
    struct Transform3DComponent;
    class Mesh3DComponent;

    class UpdateRendererSystem
    {
    public:
        static void Process(vis::SceneRenderer* scene_renderer, 
                            Mesh3DComponent& mesh_component, 
                            Transform3DComponent& transform_component,
                            Mesh3DRendererComponent& mesh_renderer_component);

    };
}

#endif