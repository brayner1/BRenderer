#include "DeviceBuffer.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>

#include <Core/LogSystem.h>


namespace brr
{
	namespace render
	{
		DeviceBuffer::DeviceBuffer() = default;

		DeviceBuffer::DeviceBuffer(size_t buffer_size, VKRD::BufferUsage buffer_usage,
		                           VKRD::MemoryUsage memory_usage, VmaAllocationCreateFlags allocation_create_flags)
	    : m_render_device(VKRD::GetSingleton()),
	      m_buffer_size(buffer_size)
		{
			BRR_LogInfo("Creating Buffer. New buffer: [ size: {} ]", buffer_size);
			m_buffer_handle = m_render_device->CreateBuffer(buffer_size, buffer_usage, memory_usage, allocation_create_flags);
		}

		DeviceBuffer::DeviceBuffer(DeviceBuffer&& device_buffer) noexcept
        {
			if (IsInitialized())
			{
				DestroyBuffer();
			}
			m_render_device = device_buffer.m_render_device;
            m_buffer_handle = device_buffer.m_buffer_handle;
		    m_buffer_size = device_buffer.m_buffer_size;

			device_buffer.m_buffer_handle = null_handle;
			device_buffer.m_buffer_size = 0;
		}

		DeviceBuffer& DeviceBuffer::operator=(DeviceBuffer&& device_buffer) noexcept
		{
			if (IsInitialized())
			{
				DestroyBuffer();
			}
			m_render_device = device_buffer.m_render_device;
			m_buffer_handle = device_buffer.m_buffer_handle;
			m_buffer_size = device_buffer.m_buffer_size;

			device_buffer.m_buffer_handle = null_handle;
			device_buffer.m_buffer_size = 0;

			return *this;
		}

		DeviceBuffer::~DeviceBuffer()
		{
			if (IsInitialized())
			{
				DestroyBuffer();
			}
		}

		void DeviceBuffer::Reset(size_t buffer_size, VKRD::BufferUsage buffer_usage,
                                 VKRD::MemoryUsage memory_usage, VmaAllocationCreateFlags allocation_create_flags)
		{
			BRR_LogInfo("Resetting Buffer. New buffer: [ size: {} ]", buffer_size);
			if (IsInitialized())
			{
				DestroyBuffer();
			}

			m_render_device = VKRD::GetSingleton();
			m_buffer_handle = m_render_device->CreateBuffer(buffer_size, buffer_usage, memory_usage, allocation_create_flags);
			m_buffer_size = buffer_size;
		}

		void DeviceBuffer::WriteToBuffer(void* data, size_t size, uint32_t offset)
		{
			assert(IsInitialized() && "DeviceBuffer must be initialized before being written.");

			if (!m_mapped_ptr)
			{
			    m_mapped_ptr = m_render_device->MapBuffer(m_buffer_handle);
			}

			if (size == VK_WHOLE_SIZE)
			{
				memcpy(m_mapped_ptr, data, m_buffer_size);
			}
			else
			{
				char* mem_start = (char*)m_mapped_ptr;
				mem_start += offset;
				memcpy(mem_start, data, size);
			}
		}

		vk::DescriptorBufferInfo DeviceBuffer::GetDescriptorInfo(vk::DeviceSize size, vk::DeviceSize offset) const
		{
			assert(IsInitialized() && "DeviceBuffer must be initialized before getting its DescriptorBufferInfo.");
			assert((size == VK_WHOLE_SIZE || size <= m_buffer_size) && "Parameter size must be either 'VK_WHOLE_SIZE' or <= DeviceBuffer size  ");
			return m_render_device->GetBufferDescriptorInfo(m_buffer_handle, size, offset);
		}

        void* DeviceBuffer::Map()
        {
			return m_render_device->MapBuffer(m_buffer_handle);
        }

        void DeviceBuffer::Unmap()
        {
			m_render_device->UnmapBuffer(m_buffer_handle);
        }

        void DeviceBuffer::DestroyBuffer()
		{
			m_render_device->UnmapBuffer(m_buffer_handle);

			m_render_device->DestroyBuffer(m_buffer_handle);

			m_buffer_handle = null_handle;

			BRR_LogInfo("Buffer Destroyed");
		}
	}
}
