#include "Renderer.h"
#include "Shader.h"
#include "Renderer/RenderDedvice.h"

namespace brr::render
{
	std::unique_ptr<RenderDevice> RenderDevice::instance = nullptr;

	RenderDevice* RenderDevice::Get_RenderDevice()
	{
		if (!instance)
		{
			instance.reset(new RenderDevice());
		}
		return instance.get();
	}

	RenderDevice::RenderDevice()
	{
	}

	void RenderDevice::Init(Renderer* renderer)
	{
	}

	void RenderDevice::CreateShaderFromFilename(std::string file_name)
	{
		//Shader shader{};
		//// Load Vertex Shader
		//{
		//	std::filesystem::path file_path{ path + ".vert" };
		//	if (!file_path.has_filename())
		//	{
		//		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "'%s' is not a valid file path.");
		//		return Shader{};
		//	}

		//	std::vector<char> vertex_shader_code = files::ReadFile(path);

		//	vk::ShaderModule vertex_shader_module = Create_ShaderModule(vertex_shader_code);

		//	shader.m_pPipelineStageInfos.push_back(vk::PipelineShaderStageCreateInfo()
		//		.setStage(vk::ShaderStageFlagBits::eVertex)
		//		.setModule(vertex_shader_module)
		//		.setPName("main"));
		//}

		//// Load Fragment Shader
		//{
		//	std::filesystem::path file_path{ path + ".frag" };
		//	if (!file_path.has_filename())
		//	{
		//		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "'%s' is not a valid file path.");
		//		return Shader{};
		//	}

		//	std::vector<char> fragment_shader_code = files::ReadFile(path);

		//	vk::ShaderModule fragment_shader_module = Create_ShaderModule(fragment_shader_code);

		//	shader.m_pPipelineStageInfos.push_back(vk::PipelineShaderStageCreateInfo()
		//		.setStage(vk::ShaderStageFlagBits::eFragment)
		//		.setModule(fragment_shader_module)
		//		.setPName("main"));
		//}

		//shader.m_isValid = true;

		//return shader;
	}
}
