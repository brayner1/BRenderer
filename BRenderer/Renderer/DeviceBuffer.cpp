#include "DeviceBuffer.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>

#include <Core/LogSystem.h>


namespace brr
{
	namespace render
	{
		DeviceBuffer::DeviceBuffer() = default;

		DeviceBuffer::DeviceBuffer(vk::DeviceSize buffer_size, VulkanRenderDevice::BufferUsage buffer_usage,
		                           VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_create_flags) :
		device_(VKRD::GetSingleton()), buffer_size_(buffer_size), buffer_usage_(buffer_usage), memory_usage_(memory_usage)
		{
			BRR_LogInfo("Creating Buffer. New buffer: [ size: {} ]", buffer_size);
			device_->Create_Buffer(buffer_size_, buffer_usage_, memory_usage_, buffer_, buffer_allocation_, allocation_create_flags);
		}

		DeviceBuffer::DeviceBuffer(DeviceBuffer&& device_buffer) noexcept
	    : device_(device_buffer.device_),
          buffer_(device_buffer.buffer_),
          buffer_allocation_(device_buffer.buffer_allocation_),
          mapped_(device_buffer.mapped_),
          buffer_size_(device_buffer.buffer_size_),
          buffer_usage_(device_buffer.buffer_usage_),
		  memory_usage_(device_buffer.memory_usage_)
        {
			device_buffer.buffer_ = VK_NULL_HANDLE;
			device_buffer.buffer_allocation_ = VK_NULL_HANDLE;
			device_buffer.mapped_ = nullptr;
		}

		DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& device_buffer) noexcept
		{
			device_ = device_buffer.device_;
			buffer_size_ = device_buffer.buffer_size_;
			buffer_usage_ = device_buffer.buffer_usage_;
			buffer_ = device_buffer.buffer_;
			buffer_allocation_ = device_buffer.buffer_allocation_;
			mapped_ = device_buffer.mapped_;
			memory_usage_ = device_buffer.memory_usage_;

			device_buffer.buffer_ = VK_NULL_HANDLE;
			device_buffer.buffer_allocation_ = VK_NULL_HANDLE;
			device_buffer.mapped_ = nullptr;
			device_buffer.buffer_size_ = 0;

			return *this;
		}

		DeviceBuffer::~DeviceBuffer()
		{
			if (IsInitialized())
			{
				DestroyBuffer();
			}
		}

		void DeviceBuffer::Reset(vk::DeviceSize buffer_size, VulkanRenderDevice::BufferUsage buffer_usage,
                                 VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_create_flags)
		{
			BRR_LogInfo("Resetting Buffer. New buffer: [ size: {} ]", buffer_size);
			if (IsInitialized())
			{
				DestroyBuffer();
			}

			device_ = VKRD::GetSingleton();
			buffer_size_ = buffer_size;
			buffer_usage_ = buffer_usage;
			memory_usage_ = memory_usage;

			device_->Create_Buffer(buffer_size, buffer_usage_, memory_usage, buffer_, buffer_allocation_, allocation_create_flags);
		}

		void DeviceBuffer::Map(vk::DeviceSize size, vk::DeviceSize offset)
		{
			assert(IsInitialized() && "DeviceBuffer must be initialized before being mapped.");

			if (!mapped_)
			{
			    VmaAllocator vma_allocator = device_->GetAllocator();
				vk::Result result = (vk::Result)vmaMapMemory(vma_allocator, buffer_allocation_, &mapped_);
				if (result != vk::Result::eSuccess)
				{
					BRR_LogError("Could not create DeviceBuffer mapped memory! Result code: {}.", vk::to_string(result).c_str());
					exit(1);
				}
			}
		}

		void DeviceBuffer::Unmap()
		{
			assert(IsInitialized() && "DeviceBuffer must be initialized before being unmapped.");

			if (mapped_)
			{
				VmaAllocator vma_allocator = device_->GetAllocator();
				vmaUnmapMemory(vma_allocator, buffer_allocation_);
				mapped_ = nullptr;
			}
		}

		void DeviceBuffer::WriteToBuffer(void* data, vk::DeviceSize size, vk::DeviceSize offset)
		{
			assert(IsInitialized() && "DeviceBuffer must be initialized before being written.");
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
			assert(IsInitialized() && "DeviceBuffer must be initialized before getting its DescriptorBufferInfo.");
			assert((size == VK_WHOLE_SIZE || size <= buffer_size_) && "Parameter size must be either 'VK_WHOLE_SIZE' or <= DeviceBuffer size  ");
			return vk::DescriptorBufferInfo{ buffer_, offset, size };
		}

		void DeviceBuffer::DestroyBuffer()
		{
			Unmap();

			VmaAllocator vma_allocator = device_->GetAllocator();

			vmaDestroyBuffer(vma_allocator, buffer_, buffer_allocation_);

			buffer_size_ = 0;

			BRR_LogInfo("Buffer Destroyed");
		}
	}
}
