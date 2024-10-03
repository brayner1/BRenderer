#ifndef BRR_WINDOWCMDLISTEXECUTER_H
#define BRR_WINDOWCMDLISTEXECUTER_H

#include <Renderer/Internal/CmdList/CmdList.h>
#include <Renderer/Internal/CmdList/WindowCmdList.h>

namespace brr::render::internal
{

    class WindowCmdListExecutor
    {
    public:

        WindowCmdListExecutor(const CmdList<WindowCommand>& cmd_list)
        : m_window_cmd_list(cmd_list)
        {}

        void ExecuteCmdList();

    private:

        void ExecuteWindowCommand(const WindowCommand& window_command) const;

        const CmdList<WindowCommand>& m_window_cmd_list;

    };
}

#endif
