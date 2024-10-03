#ifndef BRR_PIPELINE_H
#define BRR_PIPELINE_H

#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/RenderEnums.h>
#include <Renderer/GpuResources/GpuResourcesHandles.h>

#include <vector>

namespace brr::render
{
    class VulkanRenderDevice;
    class Shader;

    class DevicePipeline
    {
    public:
        DevicePipeline(const Shader& shader, 
                       const std::vector<DataFormat>& color_attachment_formats,
                       DataFormat depth_attachment_format);

        DevicePipeline(DevicePipeline&& other) noexcept;

        DevicePipeline& operator=(DevicePipeline&& other) noexcept;

        ~DevicePipeline();

        void BindGraphicsPipeline();
        void BindDescriptorSet(DescriptorSetHandle descriptor_set_handle,
                               uint32_t            set_index);

        [[nodiscard]] FORCEINLINE explicit operator bool() const noexcept { return m_pipeline_handle; }

    private:

        VulkanRenderDevice* m_device;

        ResourceHandle m_pipeline_handle {};
    };

}

#endif
