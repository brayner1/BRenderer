#ifndef BRR_BUFFER_H
#define BRR_BUFFER_H

namespace brr
{
	namespace render
	{
		class DeviceBuffer
		{
		public:

			DeviceBuffer(vk::Device device, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

			DeviceBuffer(DeviceBuffer&& device_buffer) noexcept;

			~DeviceBuffer();

			void Map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);
			void Unmap();

			void WriteToBuffer(void* data, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

			vk::DescriptorBufferInfo GetDescriptorInfo(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

			vk::Buffer GetBuffer() const { return buffer; }

		private:
			vk::Device device_;

			vk::Buffer buffer = VK_NULL_HANDLE;
			vk::DeviceMemory buffer_memory = VK_NULL_HANDLE;

			void* mapped = nullptr;

			vk::DeviceSize buffer_size;
			vk::BufferUsageFlags usage;
			vk::MemoryPropertyFlags properties;
		};
	}
}

#endif