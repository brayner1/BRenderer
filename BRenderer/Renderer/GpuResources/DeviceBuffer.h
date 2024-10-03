#ifndef BRR_BUFFER_H
#define BRR_BUFFER_H
#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>

namespace brr
{
	namespace render
	{
    class VulkanRenderDevice;

    class DeviceBuffer
    {
    public:
        DeviceBuffer();
        DeviceBuffer(size_t buffer_size, BufferUsage buffer_usage, MemoryUsage memory_usage);
        DeviceBuffer(DeviceBuffer&& device_buffer) noexcept;

        DeviceBuffer& operator=(DeviceBuffer&& device_buffer) noexcept;

        ~DeviceBuffer();

        void Reset(size_t buffer_size, BufferUsage buffer_usage, MemoryUsage memory_usage,
                   VmaAllocationCreateFlags allocation_create_flags = {});

        void WriteToBuffer(void* data, size_t size = VK_WHOLE_SIZE, uint32_t offset = 0);

        [[nodiscard]] BufferHandle GetHandle() const { return m_buffer_handle; }

        [[nodiscard]] void* Map();
        void Unmap();

        [[nodiscard]] bool IsInitialized() const { return m_render_device && m_buffer_handle; }

    private:
        void DestroyBuffer();

        VulkanRenderDevice* m_render_device = nullptr;

        BufferHandle m_buffer_handle;

        size_t m_buffer_size = 0;
        void* m_mapped_ptr = nullptr;
    };
	}
}

#endif
