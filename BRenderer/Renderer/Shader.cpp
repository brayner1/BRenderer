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

		auto createShaderModuleResult = Renderer::GetRenderer()->GetDevice()->Get_VkDevice().createShaderModule(shader_module_info);
		if (createShaderModuleResult.result != vk::Result::eSuccess)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create ShaderModule! Result code: %s.", vk::to_string(createShaderModuleResult.result).c_str());
			exit(1);
		}
		return createShaderModuleResult.value;
	}


	Shader::~Shader()
	{
		DestroyShaderModules();
	}

	Shader Shader::Create_Shader(std::string vertex_file_name, std::string frag_file_name)
	{
		Shader shader{};
		vk::ShaderModule vertex_shader_module;
		vk::ShaderModule fragment_shader_module;
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

			shader.pipeline_stage_infos_.push_back(vk::PipelineShaderStageCreateInfo()
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

			shader.pipeline_stage_infos_.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(fragment_shader_module)
				.setPName("main"));
		}

		shader.m_isValid = true;
		shader.vert_shader_module_ = vertex_shader_module;
		shader.frag_shader_module_ = fragment_shader_module;

		return std::move(shader);
	}

	void Shader::DestroyShaderModules()
	{
		if (vert_shader_module_)
		{
			Renderer::GetRenderer()->GetDevice()->Get_VkDevice().destroyShaderModule(vert_shader_module_);
		}
		vert_shader_module_ = VK_NULL_HANDLE;
		if (frag_shader_module_)
		{
			Renderer::GetRenderer()->GetDevice()->Get_VkDevice().destroyShaderModule(frag_shader_module_);
		}
		frag_shader_module_ = VK_NULL_HANDLE;
		SDL_Log("Shader modules destroyed.");
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
	}
}
