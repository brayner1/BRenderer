#include "DeviceImage.h"

namespace brr::render
{
    DeviceImage::DeviceImage()
    {
    }

    DeviceImage::DeviceImage(size_t width, size_t height, VulkanRenderDevice::ImageUsage image_usage)
        : m_width(width), m_heigth(height),
          m_device(VKRD::GetSingleton())
    {
        BRR_LogInfo("Creating DeviceImage. New Image: [ width: {}, height: {} ]", width, height);
        m_texture_handle = m_device->Create_Texture2D(width, height, image_usage);
    }

    DeviceImage::DeviceImage(DeviceImage&& device_buffer) noexcept
    {
        m_width = device_buffer.m_width;
        m_heigth = device_buffer.m_heigth;

        m_device = device_buffer.m_device;

        device_buffer.m_width = device_buffer.m_heigth = 0;
        device_buffer.m_device = VK_NULL_HANDLE;
    }

    DeviceImage::~DeviceImage()
    {
        DestroyImage();
    }

    bool DeviceImage::WriteBufferToImage(const DeviceBuffer& device_buffer)
    {
        return false;
    }

    void DeviceImage::DestroyImage()
    {
        m_device->DestroyTexture2D(m_texture_handle);
		BRR_LogInfo("Image Destroyed");
    }
}
