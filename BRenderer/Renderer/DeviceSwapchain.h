#ifndef BRR_SWAPCHAIN_H
#define BRR_SWAPCHAIN_H

#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/RenderResourcesHandles.h>

#include <Core/thirdpartiesInc.h>

namespace brr::vis
{
    class WindowRenderer;
}

namespace brr::render
{
    class VulkanRenderDevice;
    struct SwapchainWindowHandle;

    class DeviceSwapchain
    {
    public:

        DeviceSwapchain(vis::WindowRenderer* window_renderer, SwapchainWindowHandle window_handle, glm::uvec2 drawable_size);
        ~DeviceSwapchain();

        uint32_t AcquireNextImage();
        bool PresentCurrentImage();
        void BeginRendering();
        void EndRendering();

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