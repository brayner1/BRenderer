#include "UpdateRendererSystem.h"

#include <Visualization/SceneRenderer.h>
#include <Scene/Components.h>

using namespace brr;

void UpdateRendererSystem::Process(vis::SceneRenderer* scene_renderer, 
                                   Mesh3DComponent& mesh_component,
                                   Transform3DComponent& transform_component,
                                   Mesh3DRendererComponent& mesh_renderer_component)
{
    //TODO: Update instance transform if transform_component is RENDER_DIRY
    //TODO: Update render surfaces if surface is modified (added/removed/updated)
}
