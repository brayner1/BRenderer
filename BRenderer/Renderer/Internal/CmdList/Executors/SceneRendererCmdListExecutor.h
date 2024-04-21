#ifndef BRR_SCENERENDERERCMDLISTEXECUTOR_H
#define BRR_SCENERENDERERCMDLISTEXECUTOR_H

#include <Renderer/Internal/CmdList/CmdList.h>
#include <Renderer/Internal/CmdList/SceneRendererCmdList.h>
#include <Renderer/Allocators/SystemsOwner.h>

namespace brr::render::internal
{
    class SceneRendererCmdListExecutor
    {
    public:

        SceneRendererCmdListExecutor(const std::unordered_map<uint64_t, CmdList<SceneRendererCommand>>& scene_cmd_lists_map)
        : m_scene_cmd_list_map(scene_cmd_lists_map)
        {}

        void ExecuteCmdList();

    private:
        void ExecuteSceneRendererUpdateCommand(const SceneRendererCommand& scene_command, SceneRenderer* scene_renderer);

        const std::unordered_map<uint64_t, CmdList<SceneRendererCommand>>& m_scene_cmd_list_map;
    };
}

#endif
