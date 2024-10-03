#include "WindowCmdListExecutor.h"

#include <Renderer/Allocators/SystemsOwner.h>

namespace brr::render::internal
{
    void WindowCmdListExecutor::ExecuteCmdList()
    {
        for (auto& window_cmd : m_window_cmd_list)
        {
            ExecuteWindowCommand(window_cmd);
        }
    }

    void WindowCmdListExecutor::ExecuteWindowCommand(const WindowCommand& window_command) const
    {
        SystemOwner<WindowRenderer>& window_renderer_storage = SystemsStorage::GetWindowRendererStorage();
        switch (window_command.command)
        {
        case WindowCmdType::Create:
            {
                // TODO: Error if already exists
                if (window_renderer_storage.Owns(window_command.window_id))
                {
                    BRR_LogError("RenderThread: Failed to create Window (ID: {}). Window with this ID already exist.");
                    break;
                }
                BRR_LogDebug("RenderThread: Creating Window (ID: {})", window_command.window_id);
                std::unique_ptr<WindowRenderer> window_renderer = std::make_unique<WindowRenderer>(
                    window_command.window_id,
                    window_command.surface_cmd.window_size,
                    window_command.surface_cmd.swapchain_window_handle);

                window_renderer_storage.CreateNew(window_command.window_id, std::move(window_renderer));
                break;
            }
        case WindowCmdType::Destroy:
            {
                if (window_renderer_storage.Owns(window_command.window_id))
                {
                    BRR_LogDebug("RenderThread: Destroying Window (ID: {})", window_command.window_id);
                    window_renderer_storage.Erase(window_command.window_id);
                }
                break;
            }
        case WindowCmdType::Resize:
            {
                if (WindowRenderer* window_renderer = window_renderer_storage.GetSystem(window_command.window_id))
                {
                    BRR_LogDebug("RenderThread: Resizing Window (ID: {})", window_command.window_id);
                    window_renderer->Window_Resized(window_command.surface_cmd.window_size);
                }
                break;
            }
        case WindowCmdType::SurfaceOutdated:
            {
                if (WindowRenderer* window_renderer = window_renderer_storage.GetSystem(window_command.window_id))
                {
                    BRR_LogDebug("RenderThread: Surface lost on Window (ID: {})", window_command.window_id);
                    window_renderer->Window_SurfaceLost(window_command.surface_cmd.window_size,
                                                        window_command.surface_cmd.swapchain_window_handle);
                }
                break;
            }
        case WindowCmdType::SetScene:
            {
                const auto window_iter = window_renderer_storage.Find(window_command.window_id);
                if (window_iter == window_renderer_storage.end())
                {
                    BRR_LogError("RenderThread: Failed to set Scene (ID: {}) to Window. Window (ID: {}) does not exist.");
                    break;
                }

                WindowRenderer* window_renderer = window_iter->second.get();
                if (window_command.scene_cmd.scene_id == -1)
                {
                    BRR_LogDebug("RenderThread: Setting NULL Scene to Window (ID: {})", window_command.window_id);
                    window_renderer->SetSceneRenderer(nullptr, CameraID::NULL_ID);
                    break;
                }

                SystemOwner<SceneRenderer>& scene_renderer_storage = SystemsStorage::GetSceneRendererStorage();
                const auto scene_iter = scene_renderer_storage.Find(window_command.scene_cmd.scene_id);
                if (scene_iter == scene_renderer_storage.end())
                {
                    BRR_LogError("RenderThread: Failed to set Scene (ID: {}) to Window (ID: {}). Invalid Scene ID.");
                    break;
                    
                }

                BRR_LogDebug("RenderThread: Setting Scene (ID: {}) to Window (ID: {})",
                             window_command.scene_cmd.scene_id, window_command.window_id);
                SceneRenderer* scene_renderer = scene_iter->second.get();
                window_renderer->SetSceneRenderer(scene_renderer, window_command.scene_cmd.camera_id);
                break;
            }
        }
    }
}
