#ifndef BRR_WINDOWCMDLIST_H
#define BRR_WINDOWCMDLIST_H

#include "CmdList.h"
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/SceneObjectsIDs.h>

namespace brr::render::internal
{
    enum class WindowCmdType
    {
        Create,
        Destroy,
        SurfaceOutdated,
        Resize,
        SetScene
    };

    struct WindowCommand
    {
        static WindowCommand BuildCreateWindowCommand (uint32_t window_id, glm::uvec2 window_size, SwapchainWindowHandle swapchain_window_handle)
        {
            WindowCommand window_cmd;
            window_cmd.command = WindowCmdType::Create;
            window_cmd.window_id = window_id;
            window_cmd.surface_cmd = { .window_size = window_size, .swapchain_window_handle = swapchain_window_handle };
            return window_cmd;
        }

        static WindowCommand BuildDestroyWindowCommand (uint32_t window_id)
        {
            WindowCommand window_cmd;
            window_cmd.command = WindowCmdType::Destroy;
            window_cmd.window_id = window_id;
            window_cmd.surface_cmd = {};
            return window_cmd;
        }

        static WindowCommand BuildSurfaceLostCommand (uint32_t window_id, glm::uvec2 window_size, SwapchainWindowHandle swapchain_window_handle)
        {
            WindowCommand window_cmd;
            window_cmd.command = WindowCmdType::SurfaceOutdated;
            window_cmd.window_id = window_id;
            window_cmd.surface_cmd = { .window_size = window_size, .swapchain_window_handle = swapchain_window_handle };
            return window_cmd;
        }

        static WindowCommand BuildWindowResizedCommand (uint32_t window_id, glm::uvec2 window_size)
        {
            WindowCommand window_cmd;
            window_cmd.command = WindowCmdType::Resize;
            window_cmd.window_id = window_id;
            window_cmd.surface_cmd = { .window_size = window_size, .swapchain_window_handle = {} };
            return window_cmd;
        }

        static WindowCommand BuildWindowSetSceneCommand (uint32_t window_id, uint64_t scene_id, CameraID camera_id)
        {
            WindowCommand window_cmd;
            window_cmd.command = WindowCmdType::SetScene;
            window_cmd.window_id = window_id;
            window_cmd.scene_cmd = { .scene_id = scene_id, .camera_id = camera_id };
            return window_cmd;
        }

        WindowCmdType command;
        uint32_t  window_id;
        union
        {
            struct
            {
                glm::uvec2 window_size;
                SwapchainWindowHandle swapchain_window_handle;
            } surface_cmd;
            struct
            {
                uint64_t scene_id{static_cast<uint64_t>(-1)};
                CameraID camera_id{CameraID::NULL_ID};
            } scene_cmd;
        };

    private:

        WindowCommand()
        : command(), window_id(0), surface_cmd()
        {}
    };

    using WindowCmdList = CmdList<WindowCommand>;
}

#endif