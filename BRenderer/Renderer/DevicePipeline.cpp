#include "Renderer/DevicePipeline.h"

#include "Renderer/Shader.h"
#include "Renderer/Swapchain.h"

namespace brr::render
{
    DevicePipeline::DevicePipeline(vk::Device device, vk::DescriptorSetLayout descriptor_layout, const Shader& shader, Swapchain* swapchain)
    {
        if (!Init_GraphicsPipeline(device, descriptor_layout, shader, swapchain))
        {
			m_pipeline = VK_NULL_HANDLE;
			m_pipeline_layout = VK_NULL_HANDLE;
        }
    }

    DevicePipeline::DevicePipeline(DevicePipeline&& other)
    {
		m_pipeline = other.m_pipeline;
		other.m_pipeline = VK_NULL_HANDLE;

		m_pipeline_layout = other.m_pipeline_layout;
		other.m_pipeline_layout = VK_NULL_HANDLE;
    }

    DevicePipeline::~DevicePipeline()
    {
		DestroyPipeline();
    }

    bool DevicePipeline::Init_GraphicsPipeline(vk::Device device, vk::DescriptorSetLayout descriptor_layout, const Shader& shader, Swapchain* swapchain)
    {
		m_device = device;
		vk::PipelineVertexInputStateCreateInfo vertex_input_info = shader.GetPipelineVertexInputState();

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false);

		vk::Extent2D swapchain_extent = swapchain->GetSwapchain_Extent();

		vk::Viewport viewport{};
		viewport
			.setX(0.f)
			.setY(0.f)
			.setWidth((float)swapchain_extent.width)
			.setHeight((float)swapchain_extent.height)
			.setMinDepth(0.f)
			.setMaxDepth(1.f);

		vk::Rect2D scissor{ {0, 0}, swapchain_extent };

		vk::PipelineViewportStateCreateInfo viewport_state_info{};
		viewport_state_info
			.setViewportCount(1)
			.setViewports(viewport)
			.setScissorCount(1)
			.setScissors(scissor);

		vk::PipelineRasterizationStateCreateInfo rasterization_state_info{};
		rasterization_state_info
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1.f)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setDepthBiasEnable(false)
			.setDepthBiasConstantFactor(0.f)
			.setDepthBiasClamp(0.f)
			.setDepthBiasSlopeFactor(0.f);

		vk::PipelineMultisampleStateCreateInfo multisampling_info{};
		multisampling_info
			.setSampleShadingEnable(false)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setMinSampleShading(1.f)
			.setPSampleMask(nullptr)
			.setAlphaToCoverageEnable(false)
			.setAlphaToOneEnable(false);

		vk::PipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
			.setBlendEnable(false)
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
			.setAlphaBlendOp(vk::BlendOp::eAdd);

		vk::PipelineColorBlendStateCreateInfo color_blending_info{};
		color_blending_info
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachments(color_blend_attachment);

#if 1
		std::vector<vk::DynamicState> dynamic_states{ vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
		dynamic_state_info
			.setDynamicStates(dynamic_states);
#endif

		vk::PipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info
			.setSetLayouts(descriptor_layout);
		//.setPushConstantRanges(vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof()});

		auto createPipelineLayoutResult = m_device.createPipelineLayout(pipeline_layout_info);
		if (createPipelineLayoutResult.result != vk::Result::eSuccess)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Not able to create PipelineLayout. Result code: %s.", vk::to_string(createPipelineLayoutResult.result).c_str());
			return false;
		}
		m_pipeline_layout = createPipelineLayoutResult.value;

		vk::GraphicsPipelineCreateInfo graphics_pipeline_info{};
		graphics_pipeline_info
			.setStages(shader.GetPipelineStagesInfo())
			.setPVertexInputState(&vertex_input_info)
			.setPInputAssemblyState(&input_assembly_info)
			.setPViewportState(&viewport_state_info)
			.setPRasterizationState(&rasterization_state_info)
			.setPMultisampleState(&multisampling_info)
			.setPColorBlendState(&color_blending_info);
		graphics_pipeline_info
			.setLayout(m_pipeline_layout)
			.setRenderPass(swapchain->GetRender_Pass())
			.setSubpass(0)
			.setBasePipelineHandle(VK_NULL_HANDLE)
			.setBasePipelineIndex(-1);

		auto createGraphicsPipelineResult = m_device.createGraphicsPipeline(VK_NULL_HANDLE, graphics_pipeline_info);
		if (createGraphicsPipelineResult.result != vk::Result::eSuccess)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR: Could not create GraphicsPipeline! Result code: %s.", vk::to_string(createGraphicsPipelineResult.result).c_str());
			return false;
		}

		m_pipeline = createGraphicsPipelineResult.value;

		SDL_Log("Graphics DevicePipeline created.");

		return true;
    }

    void DevicePipeline::DestroyPipeline()
    {
		if (!m_pipeline)
		{
			return;
		}
		m_device.destroyPipeline(m_pipeline);
		m_pipeline = VK_NULL_HANDLE;
		m_device.destroyPipelineLayout(m_pipeline_layout);
		m_pipeline_layout = VK_NULL_HANDLE;
		SDL_Log("Destroyed Pipeline and PipelineLayout");
    }
}
