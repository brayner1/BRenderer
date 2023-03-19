#include "Renderer/DeviceBuffer.h"
#include "Renderer/Renderer.h"

namespace brr
{
	namespace render
	{
		DeviceBuffer::DeviceBuffer() = default;

		DeviceBuffer::DeviceBuffer(vk::Device device, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage,
		                           vk::MemoryPropertyFlags properties) :
		device_(device), buffer_size_(buffer_size), usage_(usage), properties_(properties)
		{
			SDL_Log("Creating Buffer. New buffer: { size: %d, usage: %s, properties: %s }", buffer_size, vk::to_string(usage).c_str(), vk::to_string(properties).c_str());
			Renderer::GetRenderer()->Create_Buffer(buffer_size, usage, properties, buffer_, buffer_memory_);
		}

		DeviceBuffer::DeviceBuffer(DeviceBuffer&& device_buffer) noexcept
		{
			device_ = device_buffer.device_;
			buffer_size_ = device_buffer.buffer_size_;
			usage_ = device_buffer.usage_;
			buffer_ = device_buffer.buffer_;
			buffer_memory_ = device_buffer.buffer_memory_;
			mapped_ = device_buffer.mapped_;

			device_buffer.buffer_ = VK_NULL_HANDLE;
			device_buffer.buffer_memory_ = VK_NULL_HANDLE;
			device_buffer.mapped_ = nullptr;
		}

		DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& device_buffer) noexcept
		{
			device_ = device_buffer.device_;
			buffer_size_ = device_buffer.buffer_size_;
			usage_ = device_buffer.usage_;
			buffer_ = device_buffer.buffer_;
			buffer_memory_ = device_buffer.buffer_memory_;
			mapped_ = device_buffer.mapped_;

			device_buffer.buffer_ = VK_NULL_HANDLE;
			device_buffer.buffer_memory_ = VK_NULL_HANDLE;
			device_buffer.mapped_ = nullptr;

			return *this;
		}

		DeviceBuffer::~DeviceBuffer()
		{
			if (IsValid())
			{
				DestroyBuffer();
			}
		}

		void DeviceBuffer::Reset(vk::Device device, vk::DeviceSize buffer_size, vk::BufferUsageFlags usage,
			vk::MemoryPropertyFlags properties)
		{
			SDL_Log("Resetting Buffer. New buffer: { size: %d, usage: %s, properties: %s }", buffer_size, vk::to_string(usage).c_str(), vk::to_string(properties).c_str());
			if (IsValid())
			{
				DestroyBuffer();
			}

			device_ = device;
			buffer_size_ = buffer_size;
			usage_ = usage;
			properties_ = properties;

			Renderer::GetRenderer()->Create_Buffer(buffer_size, usage, properties, buffer_, buffer_memory_);
		}

		void DeviceBuffer::Map(vk::DeviceSize size, vk::DeviceSize offset)
		{
			assert(IsValid() && "DeviceBuffer must be valid before being mapped.");

			mapped_ = device_.mapMemory(buffer_memory_, offset, size);
		}

		void DeviceBuffer::Unmap()
		{
			assert(IsValid() && "DeviceBuffer must be valid before being unmapped.");

			if (mapped_)
			{
				device_.unmapMemory(buffer_memory_);
				mapped_ = nullptr;
			}
		}

		void DeviceBuffer::WriteToBuffer(void* data, vk::DeviceSize size, vk::DeviceSize offset)
		{
			assert(IsValid() && "DeviceBuffer must be valid before being written.");
			assert(mapped_ && "DeviceBuffer must be mapped before being written.");

			if (size == VK_WHOLE_SIZE)
			{
				memcpy(mapped_, data, buffer_size_);
			}
			else
			{
				char* mem_start = (char*)mapped_;
				mem_start += offset;
				memcpy(mem_start, data, size);
			}
		}

		vk::DescriptorBufferInfo DeviceBuffer::GetDescriptorInfo(vk::DeviceSize size, vk::DeviceSize offset) const
		{
			assert(IsValid() && "DeviceBuffer must be valid before getting its DescriptorBufferInfo.");
			return vk::DescriptorBufferInfo{ buffer_, offset, size };
		}

		void DeviceBuffer::DestroyBuffer()
		{
			Unmap();
			device_.destroyBuffer(buffer_);
			device_.freeMemory(buffer_memory_);

			SDL_Log("Buffer Destroyed");
		}
	}
}