#ifndef BRR_DEVICEIMAGE_H
#define BRR_DEVICEIMAGE_H

#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>

namespace brr::render
{
    class DeviceBuffer;

    class DeviceImage
    {
    public:

        DeviceImage();
        DeviceImage(size_t width, size_t height, VulkanRenderDevice::ImageUsage image_usage);
        DeviceImage(DeviceImage&& device_buffer) noexcept;

        ~DeviceImage();

        bool WriteBufferToImage(const DeviceBuffer& device_buffer);

    private:

        void DestroyImage();

        size_t m_width, m_heigth;

        VulkanRenderDevice* m_device = nullptr;

        Texture2DHandle m_texture_handle;
    };
}

#endif