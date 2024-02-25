#include "WindowRenderer.h"

#include <Visualization/SceneRenderer.h>
#include <Visualization/Window.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
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
        m_swapchain = std::make_unique<render::DeviceSwapchain>(m_owner_window);
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

    void WindowRenderer::Record_CommandBuffer(SceneRenderer* scene_renderer)
    {
        scene_renderer->BeginRender();

        scene_renderer->UpdateDirtyInstances();

        m_swapchain->BeginRendering();

        scene_renderer->Render3D();

        m_swapchain->EndRendering();
    }

    void WindowRenderer::RenderWindow(SceneRenderer* scene_renderer)
    {
        bool result = m_swapchain->AcquireNextImage();
        if (!result)
        {
            return;
        }

        Record_CommandBuffer(scene_renderer);

        m_swapchain->PresentCurrentImage();

        return;
    }

    void WindowRenderer::Recreate_Swapchain()
    {
        m_render_device->WaitIdle();

        m_swapchain->Recreate_Swapchain();
    }

    void WindowRenderer::Destroy()
    {
        // Destroy Windows DeviceSwapchain and its Resources
        {
            m_swapchain = nullptr;
        }

        m_render_device = nullptr;

        BRR_LogInfo("WindowRenderer Destroyed");
    }
}
