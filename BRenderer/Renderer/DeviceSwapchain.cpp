#include "DeviceSwapchain.h"

#include <Core/LogSystem.h>
#include <Renderer/Vulkan/VkInitializerHelper.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/Internal/WindowRenderer.h>

namespace brr::render
{
    DeviceSwapchain::DeviceSwapchain(WindowRenderer* window_renderer,
                                     SwapchainWindowHandle window_handle,
                                     glm::uvec2 drawable_size)
    : m_render_device(VKRD::GetSingleton())
    {
        m_swapchain_handle = m_render_device->Swapchain_Create(window_renderer, window_handle, drawable_size);
    }

    uint32_t DeviceSwapchain::AcquireNextImage()
    {
        return m_render_device->Swapchain_AcquireNextImage(m_swapchain_handle);
    }

    bool DeviceSwapchain::PresentCurrentImage()
    {
        return m_render_device->Swapchain_PresentCurrentImage(m_swapchain_handle);
    }

    DeviceSwapchain::~DeviceSwapchain()
    {
        m_render_device->Swapchain_Destroy(m_swapchain_handle);
    }

    void DeviceSwapchain::Recreate_Swapchain(glm::uvec2 drawable_size)
    {
        m_render_device->Swapchain_Recreate(m_swapchain_handle, drawable_size);
    }

    std::vector<Texture2DHandle> DeviceSwapchain::GetSwapchainImages()
    {
        return m_render_device->GetSwapchainImages(m_swapchain_handle);
    }
}
