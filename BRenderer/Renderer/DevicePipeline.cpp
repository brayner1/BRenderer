#include "DevicePipeline.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/Shader.h>
#include <Renderer/Swapchain.h>

#include <Core/LogSystem.h>

namespace brr::render
{
    DevicePipeline::DevicePipeline(const Shader& shader, 
		                           const std::vector<DataFormat>& color_attachment_formats,
                                   DataFormat depth_attachment_format)
	: m_device(VKRD::GetSingleton()),
	  m_pipeline_handle(null_handle)
    {
		m_pipeline_handle = m_device->Create_GraphicsPipeline(shader, color_attachment_formats, depth_attachment_format);
    }

    DevicePipeline::DevicePipeline(DevicePipeline&& other) noexcept
    {
		m_pipeline_handle = other.m_pipeline_handle;
		other.m_pipeline_handle = null_handle;

		m_device = other.m_device;
		other.m_device = VK_NULL_HANDLE;
    }

    DevicePipeline& DevicePipeline::operator=(DevicePipeline&& other) noexcept
    {
		m_pipeline_handle = other.m_pipeline_handle;
		other.m_pipeline_handle = null_handle;

		m_device = other.m_device;
		other.m_device = VK_NULL_HANDLE;

		return *this;
    }

    DevicePipeline::~DevicePipeline()
    {
		if (!m_pipeline_handle)
		{
			return;
		}
		m_device->DestroyGraphicsPipeline(m_pipeline_handle);
		BRR_LogInfo("Destroyed DevicePipeline.");
    }

    void DevicePipeline::BindGraphicsPipeline()
    {
        m_device->Bind_GraphicsPipeline(m_pipeline_handle);
    }

    void DevicePipeline::BindDescriptorSet(DescriptorSetHandle descriptor_set_handle,
                                           uint32_t            set_index)
    {
        m_device->Bind_DescriptorSet(m_pipeline_handle, descriptor_set_handle, set_index);
    }
}
