#include "Renderer/WindowRenderer.h"

#include "Renderer/SceneRenderer.h"
#include "Renderer/VkInitializerHelper.h"
#include "Renderer/VulkanRenderDevice.h"
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

    WindowRenderer::WindowRenderer(Window* window)
    : m_pOwnerWindow(window),
      render_device_(VKRD::GetSingleton())
    {
		swapchain_ = std::make_unique<Swapchain>(m_pOwnerWindow);
		scene_renderer = std::make_unique<SceneRenderer>(m_pOwnerWindow->GetScene());

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

		m_graphics_pipeline = std::make_unique<DevicePipeline>(std::vector{2, m_pDescriptorSetLayout}, shader,
                                                               swapchain_.get());
	}

	void WindowRenderer::Init_CommandBuffers()
	{
		vk::Result alloc_result = render_device_->AllocateGraphicsCommandBuffers(VulkanRenderDevice::CommandBufferLevel::Primary, 2, m_pGraphicsCommandBuffers.data());
		if (alloc_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not allocate Graphics CommandBuffer! Result code: {}.", vk::to_string(alloc_result).c_str());
			exit(1);
		}

		BRR_LogInfo("Graphics CommandBuffer Created.");

		alloc_result = render_device_->AllocateTransferCommandBuffers(VulkanRenderDevice::CommandBufferLevel::Primary, 2, m_pTransferCommandBuffers.data());
		if (alloc_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not allocate Transfer CommandBuffer! Result code: {}.", vk::to_string(alloc_result).c_str());
			exit(1);
		}

		BRR_LogInfo("Graphics CommandBuffer Created.");
	}

    void WindowRenderer::Init_Synchronization()
    {
		for (int i = 0; i < FRAME_LAG; i++)
		{
			// Render finished semaphores
		    {
		        auto createRenderFinishedSempahoreResult = render_device_->Get_VkDevice().createSemaphore(vk::SemaphoreCreateInfo{});
		        if (createRenderFinishedSempahoreResult.result != vk::Result::eSuccess)
		        {
		            BRR_LogError("Could not create Render Finished Semaphore for swapchain! Result code: {}.", vk::to_string(createRenderFinishedSempahoreResult.result).c_str());
		            exit(1);
		        }
		        render_finished_semaphores_[i] = createRenderFinishedSempahoreResult.value;
		    }
			// Transfer finished semaphores
		    {
		        auto createTransferFinishedSempahoreResult = render_device_->Get_VkDevice().createSemaphore(vk::SemaphoreCreateInfo{});
		        if (createTransferFinishedSempahoreResult.result != vk::Result::eSuccess)
		        {
		            BRR_LogError("Could not create Transfer Finished Semaphore for swapchain! Result code: {}.", vk::to_string(createTransferFinishedSempahoreResult.result).c_str());
		            exit(1);
		        }
		        transfer_finished_semaphores_[i] = createTransferFinishedSempahoreResult.value;
		    }
		}
    }

    void WindowRenderer::BeginCommandBuffer(vk::CommandBuffer cmd_buffer) const
    {
		vk::CommandBufferBeginInfo cmd_buffer_begin_info{};

		cmd_buffer.begin(cmd_buffer_begin_info);
    }

    void WindowRenderer::BeginRenderPass(vk::CommandBuffer cmd_buffer) const
    {
        vk::ClearValue clear_value(std::array<float, 4>({ {0.2f, 0.2f, 0.2f, 1.f} }));

        vk::RenderPassBeginInfo render_pass_begin_info{};
        render_pass_begin_info
            .setRenderPass(swapchain_->GetRender_Pass())
            .setFramebuffer(swapchain_->GetFramebuffer(swapchain_->GetCurrentImageIndex()))
            .setRenderArea(vk::Rect2D{ {0, 0}, swapchain_->GetSwapchain_Extent() })
            .setClearValues(clear_value);

        cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	}

	void WindowRenderer::EndRenderPass(vk::CommandBuffer cmd_buffer) const
	{
        cmd_buffer.endRenderPass();
	}

    void WindowRenderer::EndCommandBuffer(vk::CommandBuffer cmd_buffer) const
    {
		cmd_buffer.end();
    }

    void WindowRenderer::Record_CommandBuffer(vk::CommandBuffer graphics_cmd_buffer, vk::CommandBuffer transfer_cmd_buffer)
	{
		BeginCommandBuffer(transfer_cmd_buffer);
		BeginCommandBuffer(graphics_cmd_buffer);

		scene_renderer->UpdateDirtyInstances();

		BeginRenderPass(graphics_cmd_buffer);

		scene_renderer->Render3D(*m_graphics_pipeline);

		EndRenderPass(graphics_cmd_buffer);
		EndCommandBuffer(graphics_cmd_buffer);
		EndCommandBuffer(transfer_cmd_buffer);
	}

    void WindowRenderer::BeginRenderWindow()
	{
		vk::Result result = swapchain_->AcquireNextImage(m_current_image_available_semaphore);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			swapchain_->Recreate_Swapchain();
			return;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire Swapchain image!");
		}

		uint32_t current_buffer_index = swapchain_->GetCurrentBufferIndex();

		vk::CommandBuffer current_transfer_cmd_buffer = m_pTransferCommandBuffers[current_buffer_index];
		const vk::Result transf_reset_result = current_transfer_cmd_buffer.reset();
		if (transf_reset_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not reset current transfer command buffer of window {}. Result code: {}", m_pOwnerWindow->GetWindowID(), vk::to_string(transf_reset_result).c_str());
			exit(1);
		}

        vk::CommandBuffer current_graphics_cmd_buffer = m_pGraphicsCommandBuffers[current_buffer_index];

		scene_renderer->BeginRender(current_buffer_index, m_frame_count, current_graphics_cmd_buffer, current_transfer_cmd_buffer);

        const vk::Result graph_reset_result = current_graphics_cmd_buffer.reset();
		if (graph_reset_result != vk::Result::eSuccess)
		{
			BRR_LogError("Could not reset current graphics command buffer of window {}. Result code: {}", m_pOwnerWindow->GetWindowID(), vk::to_string(graph_reset_result).c_str());
			exit(1);
		}

		Record_CommandBuffer(current_graphics_cmd_buffer, current_transfer_cmd_buffer);

		return;
	}

    void WindowRenderer::EndRenderWindow()
    {
		uint32_t current_buffer = swapchain_->GetCurrentBufferIndex();
		vk::Semaphore current_render_finished_semaphore = render_finished_semaphores_[current_buffer];
		vk::Semaphore current_transfer_finished_semaphore = transfer_finished_semaphores_[current_buffer];

		vk::Fence in_flight_fence = swapchain_->GetCurrentInFlightFence();

		vk::Result transfer_result = render_device_->SubmitTransferCommandBuffers(1, &m_pTransferCommandBuffers[current_buffer], 0, nullptr, nullptr, 1, &current_transfer_finished_semaphore, VK_NULL_HANDLE);

		std::array<vk::Semaphore, 2> wait_semaphores { m_current_image_available_semaphore, current_transfer_finished_semaphore };
		std::array<vk::PipelineStageFlags, 2> wait_stages { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eVertexInput };

		vk::Result result = render_device_->SubmitGraphicsCommandBuffers(1, &m_pGraphicsCommandBuffers[current_buffer], 2, wait_semaphores.data(), wait_stages.data(), 1, &current_render_finished_semaphore, in_flight_fence);

		swapchain_->PresentCurrentImage(current_render_finished_semaphore);
		++m_frame_count;
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
			render_device_->Get_VkDevice().destroySemaphore(transfer_finished_semaphores_[i]);
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
