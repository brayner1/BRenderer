#include "Renderer/Shader.h"
#include "Renderer/Renderer.h"
#include "Files/FilesUtils.h"
#include <filesystem>

namespace brr::render
{
	vk::ShaderModule Create_ShaderModule(std::vector<char>& code)
	{
		vk::ShaderModuleCreateInfo shader_module_info{};
		shader_module_info
			.setCodeSize(code.size())
			.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

		return Renderer::GetRenderer()->Get_VkDevice().createShaderModule(shader_module_info);
	}


	Shader::~Shader()
	{
		DestroyShaderModules();
	}

	Shader Shader::Create_Shader(std::string vertex_file_name, std::string frag_file_name)
	{
		Shader shader{};
		vk::ShaderModule vertex_shader_module {};
		vk::ShaderModule fragment_shader_module {};
		// Load Vertex Shader
		{
			std::filesystem::path file_path{ vertex_file_name + ".spv" };
			if (!file_path.has_filename())
			{
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "'%s' is not a valid file path.");
				return Shader{};
			}

			std::vector<char> vertex_shader_code = files::ReadFile(file_path.string());

			vertex_shader_module = Create_ShaderModule(vertex_shader_code);

			shader.m_pPipelineStageInfos.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(vertex_shader_module)
				.setPName("main"));
		}

		// Load Fragment Shader
		{
			std::filesystem::path file_path{ frag_file_name + ".spv" };
			if (!file_path.has_filename())
			{
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "'%s' is not a valid file path.");
				return Shader{};
			}

			std::vector<char> fragment_shader_code = files::ReadFile(file_path.string());

			fragment_shader_module = Create_ShaderModule(fragment_shader_code);

			shader.m_pPipelineStageInfos.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(fragment_shader_module)
				.setPName("main"));
		}

		shader.m_isValid = true;
		shader.m_pVert_shader_module = vertex_shader_module;
		shader.m_pFrag_shader_module = fragment_shader_module;

		return std::move(shader);
	}

	void Shader::DestroyShaderModules()
	{
		if (m_pVert_shader_module)
		{
			Renderer::GetRenderer()->Get_VkDevice().destroyShaderModule(m_pVert_shader_module);
		}
		m_pVert_shader_module = VK_NULL_HANDLE;
		if (m_pFrag_shader_module)
		{
			Renderer::GetRenderer()->Get_VkDevice().destroyShaderModule(m_pFrag_shader_module);
		}
		m_pFrag_shader_module = VK_NULL_HANDLE;
		SDL_Log("Shader modules destroyed.");
	}

	Shader::Shader() : m_isValid(false)
	{
	}

	Shader::Shader(Shader&& other) noexcept
	{
		m_isValid = other.m_isValid;
		m_pVert_shader_module = other.m_pVert_shader_module;
		other.m_pVert_shader_module = VK_NULL_HANDLE;
		m_pFrag_shader_module = other.m_pFrag_shader_module;
		other.m_pFrag_shader_module = VK_NULL_HANDLE;

		m_pPipelineStageInfos = std::move(other.m_pPipelineStageInfos);
	}
}
