#ifndef BRR_SHADER_H
#define BRR_SHADER_H
#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/RenderEnums.h>

#include <vector>


namespace brr::render
{
    struct DescriptorLayout;
    class DevicePipeline;
    class VulkanRenderDevice;
    class Shader;

    class ShaderBuilder
    {
    public:
        ShaderBuilder() {}

        ShaderBuilder& SetVertexShaderFile(std::string vert_shader_path);

        ShaderBuilder& SetFragmentShaderFile(std::string frag_shader_path);

        ShaderBuilder& AddVertexInputBindingDescription(uint32_t binding,
                                                        uint32_t stride);

        ShaderBuilder& AddVertexAttributeDescription(uint32_t   binding,
                                                     uint32_t   location,
                                                     DataFormat format,
                                                     uint32_t   offset);

        ShaderBuilder& AddSet();

        ShaderBuilder& AddSetBinding(DescriptorType descriptor_type, ShaderStageFlag stage_flag);

        Shader BuildShader();

    private:
        struct VertexInputBindingDesc
        {
            uint32_t m_binding;
            uint32_t m_stride;
            // TODO: Input rate (Vertex or Instance).
        };

        struct VertexInputAttributeDesc
        {
            uint32_t   m_binding;
            uint32_t   m_location;
            DataFormat m_format;
            uint32_t   m_offset;
        };

        struct SetBinding
        {
            DescriptorType m_descriptor_type;
            ShaderStageFlag m_shader_stage_flag;
        };

        struct SetLayout
        {
            std::vector<SetBinding> m_set_bindings;
        };

        std::vector<char> m_vertex_shader_code;
        std::vector<char> m_fragment_shader_code;

        std::vector<VertexInputBindingDesc>   m_binding_descs;
        std::vector<VertexInputAttributeDesc> m_attribute_descs;

        std::vector<SetLayout> m_sets_layouts;
    };

    class Shader
    {
        friend class VulkanRenderDevice;
        friend class ShaderBuilder;
    public:
        Shader();

        Shader(Shader&& other) noexcept;

        Shader& operator=(Shader&& other) noexcept;

        ~Shader();

        void DestroyShaderModules();

        [[nodiscard]] bool IsValid() const { return m_isValid; }

        [[nodiscard]] const std::vector<vk::PipelineShaderStageCreateInfo>& GetPipelineStagesInfo() const { return pipeline_stage_infos_; }

        [[nodiscard]] vk::PipelineVertexInputStateCreateInfo GetPipelineVertexInputState() const;

        [[nodiscard]] const std::vector<DescriptorLayout>& GetDescriptorSetLayouts() const { return m_descriptors_layouts; }

    private:

        VulkanRenderDevice* m_pDevice;

        bool m_isValid = false;
        vk::ShaderModule m_vert_shader_module {};
        vk::ShaderModule m_frag_shader_module {};
        std::vector<vk::PipelineShaderStageCreateInfo> pipeline_stage_infos_;

        std::vector<vk::VertexInputBindingDescription>   m_vertex_input_binding_descriptions;
        std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;

        std::vector<DescriptorLayout> m_descriptors_layouts;
    };
}

#endif