#ifndef BRR_RENDERUPDATECMDS_H
#define BRR_RENDERUPDATECMDS_H

#include "WindowCmdList.h"

namespace brr::render::internal
{
    struct RenderUpdateCmdGroup
    {
        WindowCmdList window_cmd_list;
    };
}

#endif