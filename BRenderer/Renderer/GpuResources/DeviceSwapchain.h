#ifndef BRR_SWAPCHAIN_H
#define BRR_SWAPCHAIN_H

#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/GpuResources/GpuResourcesHandles.h>

#include <Core/thirdpartiesInc.h>

namespace brr::render
{
    class VulkanRenderDevice;
    class WindowRenderer;
    struct SwapchainWindowHandle;

    class DeviceSwapchain
    {
    public:

        DeviceSwapchain(WindowRenderer* window_renderer, SwapchainWindowHandle window_handle, glm::uvec2 drawable_size);
        ~DeviceSwapchain();

        uint32_t AcquireNextImage();
        bool PresentCurrentImage();

        void Recreate_Swapchain(glm::uvec2 drawable_size);

        std::vector<Texture2DHandle> GetSwapchainImages();

        constexpr SwapchainHandle GetHandle() const { return m_swapchain_handle; }

    private:
        // Device
        VulkanRenderDevice* m_render_device = nullptr;

        SwapchainHandle m_swapchain_handle {};
    };

}

#endif