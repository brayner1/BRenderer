#ifndef BRR_RESOURCECMDLISTEXECUTOR_H
#define BRR_RESOURCECMDLISTEXECUTOR_H

#include <Renderer/Internal/CmdList/CmdList.h>
#include <Renderer/Internal/CmdList/ResourceCmdList.h>

namespace brr::render::internal
{
    class ResourceCmdListExecutor
    {
    public:

        ResourceCmdListExecutor(const ResourceCmdList& resource_cmd_list)
        : m_resource_cmd_list(resource_cmd_list)
        {}

        void ExecuteCmdList();

    private:
        void ExecuteResourceCommand(const ResourceCommand& resource_command);

        const CmdList<ResourceCommand>& m_resource_cmd_list;
    };
}

#endif