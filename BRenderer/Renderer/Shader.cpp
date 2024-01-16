#include "Shader.h"

#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Core/LogSystem.h>
#include <Files/FilesUtils.h>

#include <filesystem>

static std::vector<char> ReadShaderFile(const std::string& shader_path)
{
    std::filesystem::path file_path {shader_path};
    if (!file_path.has_filename())
    {
        BRR_LogError("'{}' is not a valid shader file path.", file_path.string());
        return {};
    }

    return brr::files::ReadFile(file_path.string());
}

namespace brr::render
{
    ShaderBuilder& ShaderBuilder::SetVertexShaderFile(std::string vert_shader_path)
    {
        std::vector<char> vertex_shader_code = ReadShaderFile (vert_shader_path);
        if (vertex_shader_code.empty())
        {
            BRR_LogError ("Vertex shader file '{}' was not loaded correctly. "
                          "Please check if path is valid or if shader is correctly compiled.", 
                          vert_shader_path);
            return *this;
        }

        m_vertex_shader_code = std::move(vertex_shader_code);
        return *this;
    }

    ShaderBuilder& ShaderBuilder::SetFragmentShaderFile(std::string frag_shader_path)
    {
        std::vector<char> fragment_shader_code = ReadShaderFile(frag_shader_path);
        if (fragment_shader_code.empty())
        {
            BRR_LogError("Fragment shader file '{}' was not loaded correctly. "
                         "Please check if path is valid or if shader is correctly compiled.",
                         frag_shader_path);
            return *this;
        }

        m_fragment_shader_code = std::move(fragment_shader_code);
        return *this;
    }

    ShaderBuilder& ShaderBuilder::AddVertexInputBindingDescription(uint32_t binding,
                                                                   uint32_t stride)
    {
        m_binding_descs.emplace_back (binding, stride);
        return *this;
    }

    ShaderBuilder& ShaderBuilder::AddVertexAttributeDescription(uint32_t   binding,
                                                                uint32_t   location,
                                                                DataFormat format,
                                                                uint32_t   offset)
    {
        m_attribute_descs.emplace_back (binding, location, format, offset);
        return *this;
    }

    Shader ShaderBuilder::BuildShader()
    {
        if (m_binding_descs.empty() && !m_attribute_descs.empty())
        {
            BRR_LogError ("Aborting invalid shader build. "
                          "ShaderBuilder have 0 Vertex Binding Descriptions, but have {} Vertex Attribute Descriptions.", 
                          m_attribute_descs.size());
            return {};
        }

        vk::Device vk_device = VKRD::GetSingleton()->Get_VkDevice();
        Shader shader;
        shader.m_pDevice = VKRD::GetSingleton();
        // Create vertex shader module
        {
            vk::ShaderModuleCreateInfo shader_module_info{};
            shader_module_info
                .setCodeSize(m_vertex_shader_code.size())
                .setPCode(reinterpret_cast<const uint32_t*>(m_vertex_shader_code.data()));

            auto createShaderModuleResult = vk_device.createShaderModule(shader_module_info);
            if (createShaderModuleResult.result != vk::Result::eSuccess)
            {
                BRR_LogError("Could not create Vertex Shader Module! Result code: {}.",
                             vk::to_string(createShaderModuleResult.result).c_str());
                return {};
            }
            shader.m_vert_shader_module = createShaderModuleResult.value;

            shader.pipeline_stage_infos_.push_back(vk::PipelineShaderStageCreateInfo()
                                                       .setStage(vk::ShaderStageFlagBits::eVertex)
                                                       .setModule(shader.m_vert_shader_module)
                                                       .setPName("main"));
        }
        // Create fragment shader module
        {
            vk::ShaderModuleCreateInfo shader_module_info{};
            shader_module_info
                .setCodeSize(m_fragment_shader_code.size())
                .setPCode(reinterpret_cast<const uint32_t*>(m_fragment_shader_code.data()));

            auto createShaderModuleResult = vk_device.createShaderModule(shader_module_info);
            if (createShaderModuleResult.result != vk::Result::eSuccess)
            {
                BRR_LogError("Could not create Vertex Shader Module! Result code: {}.",
                             vk::to_string(createShaderModuleResult.result).c_str());
                return {};
            }
            shader.m_frag_shader_module = createShaderModuleResult.value;

            shader.pipeline_stage_infos_.push_back(vk::PipelineShaderStageCreateInfo()
                                                       .setStage(vk::ShaderStageFlagBits::eFragment)
                                                       .setModule(shader.m_frag_shader_module)
                                                       .setPName("main"));
        }

        std::vector<vk::VertexInputBindingDescription> vertex_input_binding_descriptions;
        vertex_input_binding_descriptions.reserve (m_binding_descs.size());
        for (const VertexInputBindingDesc& binding_desc : m_binding_descs)
        {
            vertex_input_binding_descriptions.emplace_back (binding_desc.m_binding,
                                                            binding_desc.m_stride,
                                                            vk::VertexInputRate::eVertex);
        }
        shader.m_vertex_input_binding_descriptions = std::move(vertex_input_binding_descriptions);

        std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;
        vertex_input_attribute_descriptions.reserve (m_attribute_descs.size());
        for (const VertexInputAttributeDesc& attribute_desc : m_attribute_descs)
        {
            vk::Format format = VKRD::VkFormatFromDeviceDataFormat (attribute_desc.m_format);
            vertex_input_attribute_descriptions.emplace_back (attribute_desc.m_location,
                                                              attribute_desc.m_binding,
                                                              format,
                                                              attribute_desc.m_offset);
        }
        shader.m_vertex_input_attribute_descriptions = std::move(vertex_input_attribute_descriptions);

        // TODO: User defined descriptor sets layouts
        shader.m_descriptor_set_layouts.resize(3);
        {
            render::DescriptorLayoutBuilder layoutBuilder = shader.m_pDevice->GetDescriptorLayoutBuilder();
            layoutBuilder.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

            render::DescriptorLayout descriptor_layout = layoutBuilder.BuildDescriptorLayout();
            shader.m_descriptor_set_layouts[0]         = descriptor_layout.m_descriptor_set_layout;
        }
        {
            render::DescriptorLayoutBuilder layoutBuilder = shader.m_pDevice->GetDescriptorLayoutBuilder();
            layoutBuilder.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

            render::DescriptorLayout descriptor_layout = layoutBuilder.BuildDescriptorLayout();
            shader.m_descriptor_set_layouts[1]         = descriptor_layout.m_descriptor_set_layout;
        }
        {
            render::DescriptorLayoutBuilder layoutBuilder = shader.m_pDevice->GetDescriptorLayoutBuilder();
            layoutBuilder.SetBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

            render::DescriptorLayout descriptor_layout = layoutBuilder.BuildDescriptorLayout();
            shader.m_descriptor_set_layouts[2]         = descriptor_layout.m_descriptor_set_layout;
        }

        //shader.m_graphics_pipeline = std::make_unique<DevicePipeline> (shader.m_descriptor_set_layouts, shader, )

        shader.m_isValid = true;
        return std::move(shader);
    }

    Shader::~Shader()
    {
        DestroyShaderModules();
    }

    void Shader::DestroyShaderModules()
    {
        if (m_vert_shader_module)
        {
            m_pDevice->Get_VkDevice().destroyShaderModule(m_vert_shader_module);
        }
        m_vert_shader_module = VK_NULL_HANDLE;
        if (m_frag_shader_module)
        {
            m_pDevice->Get_VkDevice().destroyShaderModule(m_frag_shader_module);
        }
        m_frag_shader_module = VK_NULL_HANDLE;
        BRR_LogInfo("Shader modules destroyed.");
    }

    vk::PipelineVertexInputStateCreateInfo Shader::GetPipelineVertexInputState() const
    {
        vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info
            .setVertexBindingDescriptions(m_vertex_input_binding_descriptions)
            .setVertexAttributeDescriptions(m_vertex_input_attribute_descriptions);

        return vertex_input_info;
    }

    Shader::Shader()
    : m_isValid(false),
      m_pDevice (VKRD::GetSingleton())
    {
    }

    Shader::Shader(Shader&& other) noexcept
    {
        m_isValid = other.m_isValid;
        m_vert_shader_module = other.m_vert_shader_module;
        other.m_vert_shader_module = VK_NULL_HANDLE;
        m_frag_shader_module = other.m_frag_shader_module;
        other.m_frag_shader_module = VK_NULL_HANDLE;

        pipeline_stage_infos_ = std::move(other.pipeline_stage_infos_);

        m_vertex_input_binding_descriptions = std::move(other.m_vertex_input_binding_descriptions);
        other.m_vertex_input_binding_descriptions.clear();

        m_vertex_input_attribute_descriptions = std::move(other.m_vertex_input_attribute_descriptions);
        other.m_vertex_input_attribute_descriptions.clear();

        m_descriptor_set_layouts = std::move(other.m_descriptor_set_layouts);
        other.m_descriptor_set_layouts.clear();
        
        m_pDevice = other.m_pDevice;
        other.m_pDevice = nullptr;
    }
}
