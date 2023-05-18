#include "Renderer/WindowRenderer.h"

#include "Renderer/SceneRenderer.h"
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

    WindowRenderer::WindowRenderer(Window* window, RenderDevice* device)
    : m_pOwnerWindow(window),
      render_device_(device)
    {
		swapchain_ = std::make_unique<Swapchain>(render_device_, m_pOwnerWindow);
		scene_renderer = std::make_unique<SceneRenderer>(render_device_, m_pOwnerWindow->GetScene());

		// Create DescriptorPool and the DescriptorSets
		Init_DescriptorLayouts();

		Init_GraphicsPipeline();

		// Initialize CommandBuffers
		Init_CommandBuffers();

		Init_Synchronization();
    }

	WindowRenderer::~WindowRenderer()
	{
		if (render_device_)
			render_device_->WaitIdle();
		Reset();
	}

	void WindowRenderer::Window_Resized()
	{
		Recreate_Swapchain();
	}

	void WindowRenderer::Init_DescriptorLayouts()
	{
		DescriptorLayoutBuilder layoutBuilder = render_device_->GetDescriptorLayoutBuilder();
		layoutBuilder
			.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

		DescriptorLayout descriptor_layout = layoutBuilder.BuildDescriptorLayout();
		m_pDescriptorSetLayout = descriptor_layout.m_descriptor_set_layout;
	}

	void WindowRenderer::Init_GraphicsPipeline()
	{
		Shader shader = render_device_->CreateShaderFromFiles("vert", "frag");

		m_graphics_pipeline = std::make_unique<DevicePipeline>(render_device_,
                                                               std::vector{2, m_pDescriptorSetLayout}, shader,
                                                               swapchain_.get());
	}

	void WindowRenderer::Init_CommandBuffers()
	{
		vk::Result alloc_result = render_device_->AllocateGraphicsCommandBuffer(RenderDevice::CommandBufferLevel::Primary, 2, m_pCommandBuffers);
		if (alloc_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not allocate Graphics CommandBuffer! Result code: {}.", vk::to_string(alloc_result).c_str());
			exit(1);
		}

		BRR_LogInfo("CommandBuffer Created.");
	}

    void WindowRenderer::Init_Synchronization()
    {
		for (int i = 0; i < FRAME_LAG; i++)
		{
		    auto createRenderFinishedSempahoreResult = render_device_->Get_VkDevice().createSemaphore(vk::SemaphoreCreateInfo{});
		    if (createRenderFinishedSempahoreResult.result != vk::Result::eSuccess)
		    {
		        BRR_LogError("Could not create Render Finished Semaphore for swapchain! Result code: {}.", vk::to_string(createRenderFinishedSempahoreResult.result).c_str());
		        exit(1);
		    }
		    render_finished_semaphores_[i] = createRenderFinishedSempahoreResult.value;
		}
    }

    void WindowRenderer::BeginRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer) const
    {
		vk::CommandBufferBeginInfo cmd_buffer_begin_info{};

		cmd_buffer.begin(cmd_buffer_begin_info);

		vk::ClearValue clear_value(std::array<float, 4>({ {0.2f, 0.2f, 0.2f, 1.f} }));

		vk::RenderPassBeginInfo render_pass_begin_info{};
		render_pass_begin_info
			.setRenderPass(swapchain_->GetRender_Pass())
			.setFramebuffer(swapchain_->GetFramebuffer(swapchain_->GetCurrentImageIndex()))
			.setRenderArea(vk::Rect2D{ {0, 0}, swapchain_->GetSwapchain_Extent() })
			.setClearValues(clear_value);

		cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	}

	void WindowRenderer::EndRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer) const
	{
		cmd_buffer.endRenderPass();

		cmd_buffer.end();
	}

	void WindowRenderer::Record_CommandBuffer(vk::CommandBuffer cmd_buffer)
	{
		BeginRenderPass_CommandBuffer(cmd_buffer);

		scene_renderer->Render3D(cmd_buffer, swapchain_->GetCurrentBufferIndex(), *m_graphics_pipeline);

		EndRenderPass_CommandBuffer(cmd_buffer);
	}

	vk::CommandBuffer WindowRenderer::BeginRenderWindow()
	{
		vk::Result result = swapchain_->AcquireNextImage(m_current_image_available_semaphore);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			swapchain_->Recreate_Swapchain();
			return VK_NULL_HANDLE;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire Swapchain image!");
		}

		scene_renderer->UpdateDirtyInstances(swapchain_->GetCurrentBufferIndex());

		vk::CommandBuffer current_cmd_buffer = m_pCommandBuffers[swapchain_->GetCurrentBufferIndex()];

        const vk::Result reset_result = current_cmd_buffer.reset();
		if (reset_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not reset current command buffer of window {}. Result code: {}", m_pOwnerWindow->GetWindowID(), vk::to_string(reset_result).c_str());
			exit(1);
		}

		Record_CommandBuffer(current_cmd_buffer);

		return current_cmd_buffer;
	}

    void WindowRenderer::EndRenderWindow(vk::CommandBuffer cmd_buffer)
    {
		vk::Semaphore current_render_finished_semaphore = render_finished_semaphores_[swapchain_->GetCurrentBufferIndex()];

		vk::PipelineStageFlags wait_stage{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submit_info{};
		submit_info
			.setCommandBuffers(cmd_buffer)
			.setWaitSemaphores(m_current_image_available_semaphore)
			.setWaitDstStageMask(wait_stage)
			.setSignalSemaphores(current_render_finished_semaphore);


		vk::Fence in_flight_fence = swapchain_->GetCurrentInFlightFence();

		render_device_->GetGraphicsQueue().submit(submit_info, in_flight_fence);

		swapchain_->PresentCurrentImage(current_render_finished_semaphore);
    }

	void WindowRenderer::Recreate_Swapchain()
	{
		render_device_->WaitIdle();

		swapchain_->Recreate_Swapchain();
	}

	void WindowRenderer::Reset()
	{
		for (uint32_t i = 0; i < FRAME_LAG; i++)
		{
			render_device_->Get_VkDevice().destroySemaphore(render_finished_semaphores_[i]);
		}

		// Destroy Windows Swapchain and its Resources
		{
		    swapchain_ = nullptr;
		    scene_renderer.reset();
		}

		m_graphics_pipeline->DestroyPipeline();

		render_device_ = nullptr;

		BRR_LogInfo("WindowRenderer Destroyed");
	}
}
