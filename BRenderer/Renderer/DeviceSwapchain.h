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

        bool AcquireNextImage();
        bool PresentCurrentImage();
        void BeginRendering();
        void EndRendering();

        void Recreate_Swapchain();

    private:
        // Device
        VulkanRenderDevice* m_render_device = nullptr;

        ResourceHandle m_swapchain_handle {};
    };

}

#endif