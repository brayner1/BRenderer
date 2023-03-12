#ifndef BRR_BUFFER_H
#define BRR_BUFFER_H

namespace brr
{
	namespace render
	{
		class DeviceBuffer
		{
		public:

			DeviceBuffer();
			DeviceBuffer(vk::Device device, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
			DeviceBuffer(DeviceBuffer&& device_buffer) noexcept;

			~DeviceBuffer();

			void Reset(vk::Device device, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

			void Map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
			void Unmap();

			void WriteToBuffer(void* data, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

			[[nodiscard]] vk::DescriptorBufferInfo GetDescriptorInfo(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) const;

			[[nodiscard]] vk::Buffer GetBuffer() const { return buffer_; }

			inline bool IsValid() const { return device_ && buffer_; }
			operator bool() const { return IsValid(); }

		private:

			void DestroyBuffer();

			vk::Device device_ = VK_NULL_HANDLE;

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