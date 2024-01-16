#include "WindowRenderer.h"

#include <Visualization/SceneRenderer.h>
#include <Visualization/Window.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/Swapchain.h>
#include <Renderer/Shader.h>
#include <Core/LogSystem.h>


namespace brr::vis
{

    struct ModelMatrixPushConstant
    {
        glm::mat4 model_matrix;
    };

    WindowRenderer::WindowRenderer(Window* window)
    : m_owner_window(window),
      m_render_device(render::VKRD::GetSingleton())
    {
        m_swapchain = std::make_unique<render::Swapchain>(m_owner_window);
    }

    WindowRenderer::~WindowRenderer()
    {
        if (m_render_device)
            m_render_device->WaitIdle();
        Destroy();
    }

    void WindowRenderer::Window_Resized()
    {
        Recreate_Swapchain();
    }

    void WindowRenderer::Record_CommandBuffer(vk::CommandBuffer graphics_cmd_buffer, SceneRenderer* scene_renderer)
    {
        scene_renderer->BeginRender();

        scene_renderer->UpdateDirtyInstances();

        m_swapchain->BeginRendering(graphics_cmd_buffer);

        scene_renderer->Render3D();

        m_swapchain->EndRendering(graphics_cmd_buffer);
    }

    void WindowRenderer::RenderWindow(SceneRenderer* scene_renderer)
    {
        vk::Result result = m_swapchain->AcquireNextImage(m_current_image_available_semaphore);

        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            m_swapchain->Recreate_Swapchain();
            return;
        }
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("Failed to acquire Swapchain image!");
        }

        m_frame_count = m_render_device->BeginFrame();

        vk::CommandBuffer current_graphics_cmd_buffer = m_render_device->GetCurrentGraphicsCommandBuffer();

        Record_CommandBuffer(current_graphics_cmd_buffer, scene_renderer);

        vk::Fence in_flight_fence = m_swapchain->GetCurrentInFlightFence();

        vk::Semaphore render_finished_semaphore = m_render_device->EndFrame (m_current_image_available_semaphore,
                                                                             in_flight_fence);

        m_swapchain->PresentCurrentImage(render_finished_semaphore);

        return;
    }

    void WindowRenderer::Recreate_Swapchain()
    {
        m_render_device->WaitIdle();

        m_swapchain->Recreate_Swapchain();
    }

    void WindowRenderer::Destroy()
    {
        // Destroy Windows Swapchain and its Resources
        {
            m_swapchain = nullptr;
        }

        m_render_device = nullptr;

        BRR_LogInfo("WindowRenderer Destroyed");
    }
}
