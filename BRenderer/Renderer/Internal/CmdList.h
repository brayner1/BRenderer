#ifndef BRR_CMDLIST_H
#define BRR_CMDLIST_H

#include <Renderer/RenderDefs.h>

#include <array>
#include <vector>

namespace brr::render::internal
{
    // TODO: Add lock-free queues (mpmc and spsc)
    // TODO: Create circular ring buffer with FRAME_LAG slots
    // TODO: Each slot contains the set of CommandLists necessary for a frame
    // Command Lists to define:
    // WindowCmdList
    // SceneDataCmdList (camera, background, uniforms, etc)
    // SceneLightCmdList
    // SceneTexturesCmdList
    // SceneMaterialCmdList
    // SceneGeometryCmdList (vertex and index buffers)

    template <typename T, typename Alloc = std::allocator<T>>
    using CmdList = std::vector<T, Alloc>;
}

#endif