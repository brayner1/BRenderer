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

		[[nodiscard]] const std::vector<vk::PipelineShaderStageCreateInfo>& GetPipelineStagesInfo() const { return pipeline_stage_infos_; }

		[[nodiscard]] vk::PipelineVertexInputStateCreateInfo GetPipelineVertexInputState() const;

	private:
		Shader();

		Shader(Shader&& other) noexcept;

		bool m_isValid = false;
		vk::ShaderModule vert_shader_module_ {};
		vk::ShaderModule frag_shader_module_ {};
		std::vector<vk::PipelineShaderStageCreateInfo> pipeline_stage_infos_;

		vk::VertexInputBindingDescription vertex_input_binding_description_;
		std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attribute_descriptions_;

		std::vector<vk::DescriptorSetLayout>  descriptor_set_layouts_;
	};
}

#endif