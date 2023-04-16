#ifndef BRR_PIPELINE_H
#define BRR_PIPELINE_H

namespace brr::render
{
    class RenderDevice;
    class Shader;
    class Swapchain;

    class DevicePipeline
    {
    public:
        DevicePipeline() = default;
        DevicePipeline(vk::Device device, vk::DescriptorSetLayout descriptor_layout, const Shader& shader, Swapchain* swapchain);

        DevicePipeline(DevicePipeline&& other);

        ~DevicePipeline();

        bool Init_GraphicsPipeline(vk::Device device, vk::DescriptorSetLayout descriptor_layout, const Shader& shader, Swapchain* swapchain);

        [[nodiscard]] vk::Pipeline GetPipeline() const{ return m_pipeline; }
        [[nodiscard]] vk::PipelineLayout GetPipelineLayout() const { return m_pipeline_layout; }

        void DestroyPipeline();

        FORCEINLINE [[nodiscard ]]explicit operator bool() const VULKAN_HPP_NOEXCEPT { return m_pipeline; }

        FORCEINLINE [[nodiscard]] bool operator!() const VULKAN_HPP_NOEXCEPT { return !m_pipeline; }

    private:
        vk::Device m_device;

        vk::PipelineLayout m_pipeline_layout;
        vk::Pipeline       m_pipeline;
    };

}

#endif
