#include "Renderer/DeviceBuffer.h"
#include "Renderer/Renderer.h"

namespace brr
{
	namespace render
	{
		DeviceBuffer::DeviceBuffer(vk::Device device, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage,
			vk::MemoryPropertyFlags properties) :
		device_(device), buffer_size(buffer_size), usage(usage), properties(properties)
		{
			SDL_Log("Creating Buffer");
			Renderer::GetRenderer()->Create_Buffer(buffer_size, usage, properties, buffer, buffer_memory);
		}

		DeviceBuffer::DeviceBuffer(DeviceBuffer&& device_buffer) noexcept
		{
			device_ = device_buffer.device_;
			buffer_size = device_buffer.buffer_size;
			usage = device_buffer.usage;
			buffer = device_buffer.buffer;
			buffer_memory = device_buffer.buffer_memory;
			mapped = device_buffer.mapped;

			device_buffer.buffer = VK_NULL_HANDLE;
			device_buffer.buffer_memory = VK_NULL_HANDLE;
			device_buffer.mapped = nullptr;
		}

		DeviceBuffer::~DeviceBuffer()
		{
			Unmap();
			device_.destroyBuffer(buffer);
			device_.freeMemory(buffer_memory);

			SDL_Log("Buffer Destroyed");

		}

		void DeviceBuffer::Map(vk::DeviceSize size, vk::DeviceSize offset)
		{
			mapped = device_.mapMemory(buffer_memory, offset, size);
		}

		void DeviceBuffer::Unmap()
		{
			if (mapped)
			{
				device_.unmapMemory(buffer_memory);
				mapped = nullptr;
			}
		}

		void DeviceBuffer::WriteToBuffer(void* data, vk::DeviceSize size, vk::DeviceSize offset)
		{
			if (size == VK_WHOLE_SIZE)
			{
				memcpy(mapped, data, buffer_size);
			}
			else
			{
				char* mem_start = (char*)mapped;
				mem_start += offset;
				memcpy(mem_start, data, size);
			}
		}

		vk::DescriptorBufferInfo DeviceBuffer::GetDescriptorInfo(vk::DeviceSize size, vk::DeviceSize offset)
		{
			return vk::DescriptorBufferInfo{ buffer, offset, size };
		}
	}
}