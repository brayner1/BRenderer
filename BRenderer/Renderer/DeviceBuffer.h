#ifndef BRR_BUFFER_H
#define BRR_BUFFER_H

namespace brr
{
	namespace render
	{
        class RenderDevice;

        class DeviceBuffer
		{
		public:

			DeviceBuffer();
			DeviceBuffer(RenderDevice& device, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
			DeviceBuffer(DeviceBuffer&& device_buffer) noexcept;

			DeviceBuffer& operator=(DeviceBuffer&& device_buffer) noexcept;

			~DeviceBuffer();

			void Reset(RenderDevice& device, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

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

			[[nodiscard]] bool IsValid() const { return device_ && buffer_; }
            explicit [[nodiscard]] operator bool() const { return IsValid(); }

		private:

			void DestroyBuffer();

			RenderDevice* device_ = nullptr;

			vk::Buffer buffer_ = VK_NULL_HANDLE;
			vk::DeviceMemory buffer_memory_ = VK_NULL_HANDLE;

			void* mapped_ = nullptr;

			vk::DeviceSize buffer_size_ {};
			vk::BufferUsageFlags usage_ {};
			vk::MemoryPropertyFlags properties_ {};
		};
	}
}

#endif