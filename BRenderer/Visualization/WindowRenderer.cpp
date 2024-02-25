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
        m_swapchain_images = m_swapchain->GetSwapchainImages();
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

        if (m_scene_renderer && m_viewport != ViewportId::NULL_ID)
        {
            m_scene_renderer->ResizeViewport(m_viewport, m_owner_window->GetWindowExtent());
        }
    }

    void WindowRenderer::Record_CommandBuffer(SceneRenderer* scene_renderer)
    {
        scene_renderer->UpdateDirtyInstances();

        scene_renderer->Render3D(m_viewport, m_swapchain_images[m_swapchain_current_image_idx]);
    }

    void WindowRenderer::RenderWindow()
    {
        m_swapchain_current_image_idx = m_swapchain->AcquireNextImage();
        if (m_swapchain_current_image_idx == -1)
        {
            return;
        }

        Record_CommandBuffer(m_scene_renderer);

        m_swapchain->PresentCurrentImage();

        return;
    }

    void WindowRenderer::SetSceneRenderer(SceneRenderer* scene_renderer)
    {
        if (m_scene_renderer == scene_renderer)
        {
            return;
        }

        if (m_scene_renderer && m_viewport != ViewportId::NULL_ID)
        {
            m_scene_renderer->RemoveViewport(m_viewport);
            m_viewport = ViewportId::NULL_ID;
        }

        m_scene_renderer = scene_renderer;
        if (m_scene_renderer)
        {
            m_viewport = scene_renderer->CreateViewport(m_owner_window->GetWindowExtent());
        }
    }

    void WindowRenderer::Recreate_Swapchain()
    {
        m_swapchain->Recreate_Swapchain();
        m_swapchain_images = m_swapchain->GetSwapchainImages();
    }

    void WindowRenderer::Destroy()
    {
        // Destroy Windows DeviceSwapchain and its Resources
        {
            m_swapchain = nullptr;
        }

        if (m_scene_renderer && m_viewport != ViewportId::NULL_ID)
        {
            m_scene_renderer->RemoveViewport(m_viewport);
        }

        m_render_device = nullptr;

        BRR_LogInfo("WindowRenderer Destroyed");
    }
}
