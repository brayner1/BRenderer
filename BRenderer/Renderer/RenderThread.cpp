#include "RenderThread.h"

#include <thread>
#include <barrier>

#include <rigtorp/SPSCQueue.h>

#include <Renderer/Internal/RenderUpdateCmdGroup.h>
#include <Renderer/Internal/WindowRenderer.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/SceneRenderer.h>

using namespace brr::render;
using namespace internal;

static std::thread s_rendering_thread = std::thread();

static std::atomic<bool> s_stop_rendering = false;

static rigtorp::SPSCQueue<RenderUpdateCmdGroup> s_available_update_queue {FRAME_LAG};
static rigtorp::SPSCQueue<RenderUpdateCmdGroup> s_render_update_queue {FRAME_LAG};
static RenderUpdateCmdGroup s_current_render_update_cmds; // RenderUpdateCmdGroup currently owned by render thread
static RenderUpdateCmdGroup s_current_game_update_cmds;   // RenderUpdateCmdGroup currently owned by game thread

static std::unordered_map<uint32_t, std::unique_ptr<WindowRenderer>> s_window_renderers {};
static std::unordered_map<uint64_t, std::unique_ptr<SceneRenderer>> s_scene_renderers {};

static VulkanRenderDevice* s_render_device = nullptr;

//TODO: Render each registered window in loop

namespace {

    void ExecuteWindowUpdateCommand(const WindowCommand& window_command)
    {
        switch (window_command.command) {
        case WindowCmdType::Create:
        {
            BRR_LogDebug("RenderThread: Creating Window with Id: {}", window_command.window_id);
            std::unique_ptr<WindowRenderer> window_renderer = std::make_unique<WindowRenderer>(
                window_command.window_id,
                window_command.surface_cmd.window_size,
                window_command.surface_cmd.swapchain_window_handle);
            s_window_renderers.emplace(window_command.window_id, std::move(window_renderer));
            break;
        }
        case WindowCmdType::Destroy:
        {
            if (s_window_renderers.contains(window_command.window_id))
            {
                BRR_LogDebug("RenderThread: Destroying Window with Id: {}", window_command.window_id);
                s_window_renderers.erase(window_command.window_id);
            }
            break;
        }
        case WindowCmdType::Resize:
        {
            if (s_window_renderers.contains(window_command.window_id))
            {
                BRR_LogDebug("RenderThread: Resizing Window with Id: {}", window_command.window_id);
                WindowRenderer* window_renderer = s_window_renderers[window_command.window_id].get();
                window_renderer->Window_Resized(window_command.surface_cmd.window_size);
            }
            break;
        }
        case WindowCmdType::SurfaceOutdated:
        {
            if (s_window_renderers.contains(window_command.window_id))
            {
                WindowRenderer* window_renderer = s_window_renderers[window_command.window_id].get();
                window_renderer->Window_SurfaceLost(window_command.surface_cmd.window_size,
                                                    window_command.surface_cmd.swapchain_window_handle);
            }
            break;
        }
        case WindowCmdType::SetScene:
        {
            const auto window_iter = s_window_renderers.find(window_command.window_id);
            const auto scene_iter  = s_scene_renderers.find(window_command.scene_cmd.scene_id);
            if (window_iter != s_window_renderers.end()
             && scene_iter != s_scene_renderers.end())
            {
                WindowRenderer* window_renderer = window_iter->second.get();
                SceneRenderer* scene_renderer   = scene_iter->second.get();
                window_renderer->SetSceneRenderer(scene_renderer);
            }
            break;
        }
        }
    }

    void ExecuteUpdateCommands (RenderUpdateCmdGroup& render_update_cmds)
    {

        for (auto& window_cmd : render_update_cmds.window_cmd_list)
        {
            ExecuteWindowUpdateCommand(window_cmd);
        }

        render_update_cmds.window_cmd_list.clear();
    }

    void RenderThread_SyncUpdate()
    {
        s_available_update_queue.push(std::move(s_current_render_update_cmds));

        RenderUpdateCmdGroup* front;
        do
        {
            front = s_render_update_queue.front();
        } while (!front);
        s_current_render_update_cmds = std::move(*front);

        s_render_update_queue.pop();
    }


    void RenderThreadLoop()
    {
        while (!s_stop_rendering)
        {
            RenderThread_SyncUpdate();

            ExecuteUpdateCommands(s_current_render_update_cmds);

            s_render_device->BeginFrame();

            for (const auto& [window_id, window] : s_window_renderers)
            {
                window->RenderWindow();
            }

            s_render_device->EndFrame();
        }
    }

    void RenderThreadFunction()
    {
        // Add 'FRAME_LAG - 1' available update cmd sets.
        // The other one is already owned by Render Thread.
        for (uint32_t idx = 0; idx < (FRAME_LAG - 1); idx++)
        {
            s_available_update_queue.emplace();
        }
        RenderThreadLoop();

        BRR_LogInfo("Stopping Rendering Thread. Destroying render device.");
        VKRD::DestroyRenderDevice();
    }
}

void RenderThread::InitializeRenderingThread(RenderAPI render_api, SDL_Window* main_window)
{
	VKRD::CreateRenderDevice(main_window);
    s_render_device = VKRD::GetSingleton();
    s_rendering_thread = std::thread(RenderThreadFunction);
}

void RenderThread::StopRenderingThread()
{
    BRR_LogDebug("Stopping rendering thread.");
    s_stop_rendering = true;
    s_rendering_thread.join();
    BRR_LogDebug("Rendering thread joined.");
}

void RenderThread::MainThread_SyncUpdate()
{
    s_render_update_queue.push(std::move(s_current_game_update_cmds));

    RenderUpdateCmdGroup* front;
    do
    {
        front = s_available_update_queue.front();
    } while (!front);
    s_current_game_update_cmds = std::move(*front);

    s_available_update_queue.pop();
}

void RenderThread::RenderCmd_InitializeWindowRenderer(SDL_Window* window_handle, glm::uvec2 window_size)
{
    SwapchainWindowHandle swapchain_window_handle = VKRD::GetSingleton()->CreateSwapchainWindowHandle(window_handle);
    const uint32_t window_id                      = SDL_GetWindowID(window_handle);
    const WindowCommand window_cmd                = WindowCommand::BuildCreateWindowCommand(window_id,
                                                                             window_size, swapchain_window_handle);
    BRR_LogDebug("Pushing RenderCmd to initialize window. Window ID: {}", window_id);
    s_current_game_update_cmds.window_cmd_list.push_back(window_cmd);
}

void RenderThread::RenderCmd_DestroyWindowRenderer(SDL_Window* window_handle)
{
    const uint32_t window_id = SDL_GetWindowID(window_handle);
    BRR_LogDebug("Pushing RenderCmd to destroy window. Window ID: {}", window_id);
    s_current_game_update_cmds.window_cmd_list.emplace_back(WindowCommand::BuildDestroyWindowCommand(window_id));
}

void RenderThread::RenderCmd_WindowSurfaceLost(SDL_Window* window_handle, glm::uvec2 window_size)
{
    SwapchainWindowHandle swapchain_window_handle = VKRD::GetSingleton()->CreateSwapchainWindowHandle(window_handle);
    const uint32_t window_id                      = SDL_GetWindowID(window_handle);
    const WindowCommand window_cmd = WindowCommand::BuildSurfaceLostCommand(window_id,
                                                                            window_size, swapchain_window_handle);
    BRR_LogDebug("Pushing RenderCmd to update surface of window. Window ID: {}", window_id);
    s_current_game_update_cmds.window_cmd_list.push_back(window_cmd);
}

void RenderThread::RenderCmd_ResizeWindow(SDL_Window* window_handle, glm::uvec2 window_size)
{
    const uint32_t window_id = SDL_GetWindowID(window_handle);
    const WindowCommand window_cmd = WindowCommand::BuildWindowResizedCommand(window_id,
                                                                              window_size);
    BRR_LogDebug("Pushing RenderCmd to resize window. Window ID: {}", window_id);
    s_current_game_update_cmds.window_cmd_list.push_back(window_cmd);
}

void RenderThread::RenderCmd_SetWindowScene(SDL_Window* window_handle, uint64_t scene_id)
{
    const uint32_t window_id = SDL_GetWindowID(window_handle);
    const WindowCommand window_cmd = WindowCommand::BuildWindowSetSceneCommand(window_id, scene_id);
    s_current_game_update_cmds.window_cmd_list.push_back(window_cmd);
}
