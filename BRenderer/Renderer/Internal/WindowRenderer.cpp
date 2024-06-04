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
    : m_window_id(window_id),
      m_window_extent(window_extent),
      m_render_device(render::VKRD::GetSingleton())
    {
        m_minimized = m_window_extent.x == 0 || m_window_extent.y == 0;
        m_swapchain = std::make_unique<render::DeviceSwapchain>(this, swapchain_window_handle, window_extent);
        m_swapchain_images = m_swapchain->GetSwapchainImages();
    }

    WindowRenderer::~WindowRenderer()
    {
        if (m_render_device)
            m_render_device->WaitIdle();
        // Destroy Windows DeviceSwapchain and its Resources
        {
            m_swapchain = nullptr;
        }

        if (m_scene_renderer && m_viewport != ViewportID::NULL_ID)
        {
            m_scene_renderer->DestroyViewport(m_viewport);
        }

        m_render_device = nullptr;

        BRR_LogInfo("WindowRenderer Destroyed");
    }

    void WindowRenderer::Device_NotifySurfaceLost() const
    {
        //TODO: Call on main thread to re-generate surface and create render command to update it
    }

    void WindowRenderer::Window_Resized(glm::uvec2 new_extent)
    {
        m_window_extent = new_extent;
        if (m_window_extent.x == 0 || m_window_extent.y == 0)
        {
            m_minimized = true;
            return;
        }
        m_minimized = false;
        Recreate_Swapchain();

        if (m_scene_renderer && m_viewport != ViewportID::NULL_ID)
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

    void WindowRenderer::Record_CommandBuffer()
    {
        if (!m_scene_renderer)
        {
            return;
        }

        m_scene_renderer->UpdateDirtyInstances();

        if (m_viewport == ViewportID::NULL_ID)
        {
            return;
        }

        m_scene_renderer->Render3D(m_viewport, m_swapchain_images[m_swapchain_current_image_idx]);
    }

    void WindowRenderer::RenderWindow()
    {
        if (m_minimized)
            return;

        m_swapchain_current_image_idx = m_swapchain->AcquireNextImage();
        if (m_swapchain_current_image_idx == -1)
        {
            BRR_LogError("Could not acquire next image of Swapchain in WindowRenderer (ID: {}).", m_window_id);
            return;
        }

        Record_CommandBuffer();

        m_swapchain->PresentCurrentImage();

        return;
    }

    void WindowRenderer::SetSceneRenderer(SceneRenderer* scene_renderer, CameraID camera_id)
    {
        if (m_scene_renderer != scene_renderer)
        {
            if (m_scene_renderer && m_viewport != ViewportID::NULL_ID)
            {
                m_scene_renderer->DestroyViewport(m_viewport);
            }
            m_viewport = ViewportID::NULL_ID;
            m_scene_renderer = scene_renderer;
        }

        if (!m_scene_renderer || camera_id == CameraID::NULL_ID)
        {
            return;
        }

        if (m_viewport == ViewportID::NULL_ID)
        {
            m_viewport = m_scene_renderer->CreateViewport(m_window_extent, camera_id);
        }
        else
        {
            m_scene_renderer->SetViewportCameraID(m_viewport, camera_id);
        }
    }

    void WindowRenderer::Recreate_Swapchain()
    {
        m_swapchain->Recreate_Swapchain(m_window_extent);
        m_swapchain_images = m_swapchain->GetSwapchainImages();
    }
}
