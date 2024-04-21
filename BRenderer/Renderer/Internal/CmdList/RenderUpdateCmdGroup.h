#ifndef BRR_RENDERUPDATECMDS_H
#define BRR_RENDERUPDATECMDS_H

#include "WindowCmdList.h"
#include "SceneRendererCmdList.h"

namespace brr::render::internal
{
    struct RenderUpdateCmdGroup
    {
        WindowCmdList window_cmd_list;
        std::unordered_map<uint64_t, SceneRendererCmdList> scene_cmd_list_map;
    };
}

#endif