#include "DeviceBuffer.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>

#include <Core/LogSystem.h>


namespace brr
{
	namespace render
	{
		DeviceBuffer::DeviceBuffer() = default;

		DeviceBuffer::DeviceBuffer(size_t buffer_size, BufferUsage buffer_usage,
		                           MemoryUsage memory_usage)
	    : m_render_device(VKRD::GetSingleton()),
	      m_buffer_size(buffer_size)
		{
			BRR_LogInfo("Creating Buffer. New buffer: [ size: {} ]", buffer_size);
			m_buffer_handle = m_render_device->CreateBuffer(buffer_size, buffer_usage, memory_usage);
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

		void DeviceBuffer::Reset(size_t buffer_size, BufferUsage buffer_usage,
                                 MemoryUsage memory_usage, VmaAllocationCreateFlags allocation_create_flags)
		{
			BRR_LogInfo("Resetting Buffer. New buffer: [ size: {} ]", buffer_size);
			if (IsInitialized())
			{
				DestroyBuffer();
			}

			m_render_device = VKRD::GetSingleton();
			m_buffer_handle = m_render_device->CreateBuffer(buffer_size, buffer_usage, memory_usage);
			m_buffer_size   = buffer_size;
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
