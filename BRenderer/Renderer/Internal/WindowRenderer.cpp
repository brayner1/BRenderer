#include "WindowRenderer.h"

#include <Renderer//SceneRenderer.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Core/LogSystem.h>


namespace brr::render
{

    struct ModelMatrixPushConstant
    {
        glm::mat4 model_matrix;
    };

    WindowRenderer::WindowRenderer(uint32_t window_id, 
                                   glm::uvec2 window_extent,
                                   render::SwapchainWindowHandle swapchain_window_handle)
    : m_render_device(render::VKRD::GetSingleton()),
      m_window_id(window_id)
    {
        m_swapchain = std::make_unique<render::DeviceSwapchain>(this, swapchain_window_handle, window_extent);
        m_swapchain_images = m_swapchain->GetSwapchainImages();
    }

    WindowRenderer::~WindowRenderer()
    {
        if (m_render_device)
            m_render_device->WaitIdle();
        Destroy();
    }

    void WindowRenderer::Device_NotifySurfaceLost() const
    {
        //TODO: Call on main thread to re-generate surface and create render command to update it
    }

    void WindowRenderer::Window_Resized(glm::uvec2 new_extent)
    {
        m_window_extent = new_extent;
        Recreate_Swapchain();

        if (m_scene_renderer && m_viewport != ViewportId::NULL_ID)
        {
            m_scene_renderer->ResizeViewport(m_viewport, m_window_extent);
        }
    }

    void WindowRenderer::Window_SurfaceLost(glm::uvec2 window_extent, render::SwapchainWindowHandle swapchain_window_handle)
    {
        m_window_extent = window_extent;
        m_swapchain = std::make_unique<render::DeviceSwapchain>(this, swapchain_window_handle, m_window_extent);
        m_swapchain_images = m_swapchain->GetSwapchainImages();
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

        //Record_CommandBuffer(m_scene_renderer);

        m_swapchain->BeginRendering();

        m_swapchain->EndRendering();

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
            m_viewport = scene_renderer->CreateViewport(m_window_extent);
        }
    }

    void WindowRenderer::Recreate_Swapchain()
    {
        m_swapchain->Recreate_Swapchain(m_window_extent);
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
