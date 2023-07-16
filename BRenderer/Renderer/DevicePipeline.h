#ifndef BRR_PIPELINE_H
#define BRR_PIPELINE_H

namespace brr::render
{
    class VulkanRenderDevice;
    class Shader;
    class Swapchain;

    class DevicePipeline
    {
    public:
        DevicePipeline(std::vector<vk::DescriptorSetLayout> descriptors_layouts, 
                       const Shader& shader, 
                       Swapchain* swapchain);

        DevicePipeline(DevicePipeline&& other) noexcept;

        DevicePipeline& operator=(DevicePipeline&& other) noexcept;


        ~DevicePipeline();

        [[nodiscard]] constexpr vk::Pipeline GetPipeline() const{ return m_pipeline; }
        [[nodiscard]] constexpr vk::PipelineLayout GetPipelineLayout() const { return m_pipeline_layout; }

        void DestroyPipeline();

        FORCEINLINE [[nodiscard]] explicit operator bool() const noexcept { return m_pipeline; }

        FORCEINLINE [[nodiscard]] bool operator!() const noexcept { return !m_pipeline; }

    private:

        bool Init_GraphicsPipeline(
            std::vector<vk::DescriptorSetLayout> descriptors_layouts,
            const Shader& shader,
            Swapchain* swapchain);

        VulkanRenderDevice* m_device;

        vk::PipelineLayout m_pipeline_layout;
        vk::Pipeline       m_pipeline;
    };

}

#endif
