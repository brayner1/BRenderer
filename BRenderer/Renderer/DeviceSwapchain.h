#ifndef BRR_SWAPCHAIN_H
#define BRR_SWAPCHAIN_H

#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/RenderDefs.h>
#include <Renderer/ResourcesHandles.h>

#include <Core/thirdpartiesInc.h>

namespace brr::vis
{
    class Window;
}

namespace brr::render
{

    class VulkanRenderDevice;

    class DeviceSwapchain
    {
    public:

        DeviceSwapchain(vis::Window* window);
        ~DeviceSwapchain();

        uint32_t AcquireNextImage();
        bool PresentCurrentImage();
        void BeginRendering();
        void EndRendering();

        void Recreate_Swapchain();

        std::vector<Texture2DHandle> GetSwapchainImages();

        constexpr SwapchainHandle GetHandle() const { return m_swapchain_handle; }

    private:
        // Device
        VulkanRenderDevice* m_render_device = nullptr;

        SwapchainHandle m_swapchain_handle {};
    };

}

#endif