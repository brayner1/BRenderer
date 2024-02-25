#include "DeviceSwapchain.h"

#include <Renderer/Vulkan/VkInitializerHelper.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Visualization/Window.h>
#include <Core/LogSystem.h>


namespace brr::render
{
    DeviceSwapchain::DeviceSwapchain(vis::Window* window) : m_render_device(VKRD::GetSingleton())
    {
        m_swapchain_handle = m_render_device->Swapchain_Create(window);
    }

    bool DeviceSwapchain::AcquireNextImage()
    {
        return m_render_device->Swapchain_AcquireNextImage(m_swapchain_handle);
    }

    bool DeviceSwapchain::PresentCurrentImage()
    {
        return m_render_device->Swapchain_PresentCurrentImage(m_swapchain_handle);
    }

    void DeviceSwapchain::BeginRendering()
    {
        m_render_device->Swapchain_BeginRendering(m_swapchain_handle);
    }

    void DeviceSwapchain::EndRendering()
    {
        m_render_device->Swapchain_EndRendering(m_swapchain_handle);
    }

    DeviceSwapchain::~DeviceSwapchain()
    {
        m_render_device->Swapchain_Destroy(m_swapchain_handle);
    }

    void DeviceSwapchain::Recreate_Swapchain()
    {
        m_render_device->Swapchain_Recreate(m_swapchain_handle);
    }
}
