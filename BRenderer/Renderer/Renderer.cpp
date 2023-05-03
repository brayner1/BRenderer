#include "Renderer/Renderer.h"

#include "Renderer/VkInitializerHelper.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/Swapchain.h"
#include "Renderer/Shader.h"
#include "Core/Window.h"
#include "Core/PerspectiveCamera.h"
#include "Core/LogSystem.h"
#include "Scene/Entity.h"


namespace brr::render
{

	struct ModelMatrixPushConstant
	{
		glm::mat4 model_matrix;
	};

    Renderer::Renderer(Window* window, RenderDevice* device)
    : m_pOwnerWindow(window),
      render_device_(device)
    {
		swapchain_ = std::make_unique<Swapchain>(render_device_, m_pOwnerWindow);
		scene_renderer = std::make_unique<SceneRenderer>(render_device_, m_pOwnerWindow->GetScene()->m_registry_);

		// Create DescriptorPool and the DescriptorSets
		Init_DescriptorLayouts();

		Init_GraphicsPipeline();

		// Initialize CommandBuffers
		Init_CommandBuffers();
    }

	Renderer::~Renderer()
	{
		if (render_device_)
			render_device_->WaitIdle();
		Reset();
	}

	void Renderer::Window_Resized()
	{
		Recreate_Swapchain();
	}

	void Renderer::Init_DescriptorLayouts()
	{
		DescriptorLayoutBuilder layoutBuilder = render_device_->GetDescriptorLayoutBuilder();
		layoutBuilder
			.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

		DescriptorLayout descriptor_layout = layoutBuilder.BuildDescriptorLayout();
		m_pDescriptorSetLayout = descriptor_layout.m_descriptor_set_layout;
	}

	void Renderer::Init_GraphicsPipeline()
	{
		Shader shader = render_device_->CreateShaderFromFiles("vert", "frag");

		m_graphics_pipeline.Init_GraphicsPipeline(render_device_->Get_VkDevice(), {2, m_pDescriptorSetLayout }, shader, swapchain_.get());
	}

	void Renderer::Init_CommandBuffers()
	{
		vk::CommandBufferAllocateInfo command_buffer_alloc_info{};
		command_buffer_alloc_info
			.setCommandPool(render_device_->GetGraphicsCommandPool())
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(2);

		auto allocCmdBufferResult = render_device_->Get_VkDevice().allocateCommandBuffers(command_buffer_alloc_info);
		if (allocCmdBufferResult.result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not allocate CommandBuffer! Result code: {}.", vk::to_string(allocCmdBufferResult.result).c_str());
			exit(1);
		}
		m_pCommandBuffers = allocCmdBufferResult.value;

		BRR_LogInfo("CommandBuffer Created.");

		if (render_device_->IsDifferentPresentQueue())
		{
			vk::CommandBufferAllocateInfo present_buffer_alloc_info{};
			present_buffer_alloc_info
				.setCommandPool(render_device_->GetPresentCommandPool())
				.setLevel(vk::CommandBufferLevel::ePrimary)
				.setCommandBufferCount(1);

			auto allocPresentCmdBufferResult = render_device_->Get_VkDevice().allocateCommandBuffers(present_buffer_alloc_info);
			if (allocPresentCmdBufferResult.result != vk::Result::eSuccess)
			{
				BRR_LogError("Could not allocate presentation CommandBuffer! Result code: {}.", vk::to_string(allocPresentCmdBufferResult.result).c_str());
				exit(1);
			}

			m_pPresentCommandBuffer = allocPresentCmdBufferResult.value[0];

			BRR_LogInfo("Separate Present CommandBuffer Created.");
		}
	}

	void Renderer::BeginRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer,
                                                 uint32_t image_index) const
    {
		vk::CommandBufferBeginInfo cmd_buffer_begin_info{};

		cmd_buffer.begin(cmd_buffer_begin_info);

		if (render_device_->IsDifferentPresentQueue())
		{
			present_cmd_buffer.begin(cmd_buffer_begin_info);
		}

		vk::ClearValue clear_value(std::array<float, 4>({ {0.2f, 0.2f, 0.2f, 1.f} }));

		vk::RenderPassBeginInfo render_pass_begin_info{};
		render_pass_begin_info
			.setRenderPass(swapchain_->GetRender_Pass())
			.setFramebuffer(swapchain_->GetFramebuffer(image_index))
			.setRenderArea(vk::Rect2D{ {0, 0}, swapchain_->GetSwapchain_Extent() })
			.setClearValues(clear_value);

		cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	}

	void Renderer::EndRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer) const
	{
		cmd_buffer.endRenderPass();

		cmd_buffer.end();
	}

	void Renderer::Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index)
	{
		BeginRenderPass_CommandBuffer(cmd_buffer, present_cmd_buffer, image_index);

		scene_renderer->Render(cmd_buffer, swapchain_->GetCurrentBuffer(), m_graphics_pipeline);

		EndRenderPass_CommandBuffer(cmd_buffer);
	}

	vk::CommandBuffer Renderer::BeginRenderWindow()
	{
		vk::Result result = swapchain_->AcquireNextImage(current_image_idx);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			swapchain_->Recreate_Swapchain();
			return VK_NULL_HANDLE;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire Swapchain image!");
		}

		Scene* scene = m_pOwnerWindow->GetScene();
		glm::mat4 projection_view = scene->GetMainCamera()->GetProjectionMatrix() * scene->GetMainCamera()->GetViewMatrix();
		scene_renderer->UpdateRenderData(scene->m_registry_, swapchain_->GetCurrentBuffer(), projection_view);

		vk::CommandBuffer current_cmd_buffer = m_pCommandBuffers[swapchain_->GetCurrentBuffer()];

        const vk::Result reset_result = current_cmd_buffer.reset();
		if (reset_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not reset current command buffer of window {}. Result code: {}", m_pOwnerWindow->GetWindowID(), vk::to_string(reset_result).c_str());
			exit(1);
		}

		vk::CommandBuffer present_cmd_buffer = (render_device_->IsDifferentPresentQueue() ?
			                                    m_pPresentCommandBuffer : current_cmd_buffer);

		Record_CommandBuffer(current_cmd_buffer, 
                             present_cmd_buffer, 
                             current_image_idx);

		return current_cmd_buffer;
	}

    void Renderer::EndRenderWindow(vk::CommandBuffer cmd_buffer)
    {
		swapchain_->SubmitCommandBuffer(cmd_buffer, current_image_idx);
    }

	void Renderer::Recreate_Swapchain()
	{
		render_device_->WaitIdle();

		swapchain_->Recreate_Swapchain();
	}

	void Renderer::Reset()
	{
		// Destroy Windows Swapchain and its Resources
		{
		    swapchain_ = nullptr;
		    scene_renderer.reset();
		}

		m_graphics_pipeline.DestroyPipeline();

		render_device_ = nullptr;

		BRR_LogInfo("Renderer Destroyed");
	}
}
