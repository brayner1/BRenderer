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
        DevicePipeline(vk::Device device, 
                       std::vector<vk::DescriptorSetLayout> descriptors_layouts, 
                       const Shader& shader, 
                       Swapchain* swapchain);

        DevicePipeline(DevicePipeline&& other) noexcept;

        ~DevicePipeline();

        bool Init_GraphicsPipeline(vk::Device device, 
                                   std::vector<vk::DescriptorSetLayout> descriptors_layouts, 
                                   const Shader& shader, 
                                   Swapchain* swapchain);

        [[nodiscard]] constexpr vk::Pipeline GetPipeline() const{ return m_pipeline; }
        [[nodiscard]] constexpr vk::PipelineLayout GetPipelineLayout() const { return m_pipeline_layout; }

        void DestroyPipeline();

        FORCEINLINE [[nodiscard]] explicit operator bool() const noexcept { return m_pipeline; }

        FORCEINLINE [[nodiscard]] bool operator!() const noexcept { return !m_pipeline; }

    private:
        vk::Device m_device;

        vk::PipelineLayout m_pipeline_layout;
        vk::Pipeline       m_pipeline;
    };

}

#endif
