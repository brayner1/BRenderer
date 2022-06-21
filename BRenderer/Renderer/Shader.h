#ifndef BRR_SHADER_H
#define BRR_SHADER_H

namespace brr::render
{
	class Shader
	{
	public:

		~Shader();

		static Shader Create_Shader(std::string vertex_file_name, std::string frag_file_name);

		void DestroyShaderModules();

		[[nodiscard]] bool IsValid() const { return m_isValid; }

		[[nodiscard]] const std::vector<vk::PipelineShaderStageCreateInfo>& GetPipelineStagesInfo() const { return m_pPipelineStageInfos; }

	private:
		Shader();

		Shader(Shader&& other) noexcept;

		bool m_isValid = false;
		vk::ShaderModule m_pVert_shader_module {};
		vk::ShaderModule m_pFrag_shader_module {};
		std::vector<vk::PipelineShaderStageCreateInfo> m_pPipelineStageInfos;
	};
}

#endif