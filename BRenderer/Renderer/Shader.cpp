#include "Renderer/Shader.h"
#include "Renderer/Renderer.h"
#include "Geometry/Geometry.h"
#include "Files/FilesUtils.h"
#include "Core/LogSystem.h"


namespace brr::render
{
	Shader::~Shader()
	{
		DestroyShaderModules();
	}

	void Shader::DestroyShaderModules()
	{
		if (vert_shader_module_)
		{
			m_pDevice->Get_VkDevice().destroyShaderModule(vert_shader_module_);
		}
		vert_shader_module_ = VK_NULL_HANDLE;
		if (frag_shader_module_)
		{
			m_pDevice->Get_VkDevice().destroyShaderModule(frag_shader_module_);
		}
		frag_shader_module_ = VK_NULL_HANDLE;
		BRR_LogInfo("Shader modules destroyed.");
	}

    vk::PipelineVertexInputStateCreateInfo Shader::GetPipelineVertexInputState() const
    {
		vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info
			.setVertexBindingDescriptions(vertex_input_binding_description_)
			.setVertexAttributeDescriptions(vertex_input_attribute_descriptions_);

		return vertex_input_info;
    }

	Shader::Shader() : m_isValid(false)
	{
	}

	Shader::Shader(Shader&& other) noexcept
	{
		m_isValid = other.m_isValid;
		vert_shader_module_ = other.vert_shader_module_;
		other.vert_shader_module_ = VK_NULL_HANDLE;
		frag_shader_module_ = other.frag_shader_module_;
		other.frag_shader_module_ = VK_NULL_HANDLE;

		pipeline_stage_infos_ = std::move(other.pipeline_stage_infos_);

		vertex_input_binding_description_ = std::move(other.vertex_input_binding_description_);
		other.vertex_input_binding_description_ = vk::VertexInputBindingDescription{};

		vertex_input_attribute_descriptions_ = std::move(other.vertex_input_attribute_descriptions_);
		other.vertex_input_attribute_descriptions_.fill({});
		
		m_pDevice = other.m_pDevice;
		other.m_pDevice = nullptr;
	}
}
