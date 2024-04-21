#include "SystemsOwner.h"

namespace brr::render
{
    SystemOwner<WindowRenderer>& SystemsStorage::GetWindowRendererStorage()
    {
        static SystemOwner<WindowRenderer> s_window_renderers {};
        return s_window_renderers;
    }

    SystemOwner<SceneRenderer>& SystemsStorage::GetSceneRendererStorage()
    {
        static SystemOwner<SceneRenderer> s_scene_renderers {};
        return s_scene_renderers;
    }
}
