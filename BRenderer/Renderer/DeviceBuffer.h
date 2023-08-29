#ifndef BRR_BUFFER_H
#define BRR_BUFFER_H
#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>

namespace brr
{
	namespace render
	{
        class VulkanRenderDevice;

        enum class MemoryUsage
		{
			// Buffer memory can be mapped and accessed in the CPU.
			// It's recommended that the reads/writes will be made in sequential order.
			// Random access to the buffer will be very slow.
		    CPU_ACCESS_SEQUENTIAL,
			// Buffer memory can be mapped and accessed in the CPU.
			// It uses a cached memory and can be read/written in random order.
			CPU_ACCESS_CACHED
		};

        class DeviceBuffer
		{
		public:

			DeviceBuffer();
			DeviceBuffer(vk::DeviceSize buffer_size, VulkanRenderDevice::BufferUsage buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_create_flags = {});
			DeviceBuffer(DeviceBuffer&& device_buffer) noexcept;

			DeviceBuffer& operator=(DeviceBuffer&& device_buffer) noexcept;

			~DeviceBuffer();

			void Reset(vk::DeviceSize buffer_size, VulkanRenderDevice::BufferUsage buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_create_flags = {});

			void Map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
			void Unmap();

			void WriteToBuffer(void* data, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

            /**
			 * \brief Get this buffer DescriptorBufferInfo.
			 * \param size The size of the
			 * \param offset 
			 * \return 
			 */
			[[nodiscard]] vk::DescriptorBufferInfo GetDescriptorInfo(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) const;

			[[nodiscard]] vk::Buffer GetBuffer() const { return buffer_; }

			[[nodiscard]] vk::DeviceSize GetBufferSize() const { return buffer_size_; }

			[[nodiscard]] bool IsInitialized() const { return device_ && buffer_; }
            explicit [[nodiscard]] operator bool() const { return IsInitialized(); }

		private:

			void DestroyBuffer();

			VulkanRenderDevice* device_ = nullptr;

			vk::Buffer buffer_ {};
			VmaAllocation buffer_allocation_ {};

			void* mapped_ = nullptr;

			vk::DeviceSize buffer_size_ {};
			VulkanRenderDevice::BufferUsage buffer_usage_ {};
			VmaMemoryUsage memory_usage_ {};
		};
	}
}

#endif