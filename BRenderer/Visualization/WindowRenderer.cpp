#include "WindowRenderer.h"

#include <Visualization/SceneRenderer.h>
#include <Visualization/Window.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>
#include <Renderer/Swapchain.h>
#include <Renderer/Shader.h>
#include <Core/LogSystem.h>


namespace brr::vis
{

	struct ModelMatrixPushConstant
	{
		glm::mat4 model_matrix;
	};

    WindowRenderer::WindowRenderer(Window* window)
    : m_pOwnerWindow(window),
      m_render_device(render::VKRD::GetSingleton())
    {
		swapchain_ = std::make_unique<render::Swapchain>(m_pOwnerWindow);
		m_scene_renderer = std::make_unique<SceneRenderer>(m_pOwnerWindow->GetScene());

		// Create DescriptorPool and the DescriptorSets
		Init_DescriptorLayouts();

		Init_GraphicsPipeline();
    }

	WindowRenderer::~WindowRenderer()
	{
		if (m_render_device)
			m_render_device->WaitIdle();
		Reset();
	}

	void WindowRenderer::Window_Resized()
	{
		Recreate_Swapchain();
	}

	void WindowRenderer::Init_DescriptorLayouts()
	{
        render::DescriptorLayoutBuilder layoutBuilder = m_render_device->GetDescriptorLayoutBuilder();
		layoutBuilder
			.SetBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

        render::DescriptorLayout descriptor_layout = layoutBuilder.BuildDescriptorLayout();
		m_pDescriptorSetLayout = descriptor_layout.m_descriptor_set_layout;
	}

	void WindowRenderer::Init_GraphicsPipeline()
	{
        render::Shader shader = m_render_device->CreateShaderFromFiles("vert", "frag");

		m_graphics_pipeline = std::make_unique<render::DevicePipeline>(std::vector{2, m_pDescriptorSetLayout}, shader,
                                                                       swapchain_.get());
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

    void WindowRenderer::Record_CommandBuffer(vk::CommandBuffer graphics_cmd_buffer, vk::CommandBuffer transfer_cmd_buffer)
	{
		m_scene_renderer->UpdateDirtyInstances();

		BeginRenderPass(graphics_cmd_buffer);

		m_scene_renderer->Render3D(*m_graphics_pipeline);

		EndRenderPass(graphics_cmd_buffer);
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

		m_frame_count = m_render_device->BeginFrame();

		uint32_t current_buffer_index = swapchain_->GetCurrentBufferIndex();

		vk::CommandBuffer current_transfer_cmd_buffer = m_render_device->GetCurrentTransferCommandBuffer();
        vk::CommandBuffer current_graphics_cmd_buffer = m_render_device->GetCurrentGraphicsCommandBuffer();

		m_scene_renderer->BeginRender(current_buffer_index, m_frame_count, current_graphics_cmd_buffer, current_transfer_cmd_buffer);

		Record_CommandBuffer(current_graphics_cmd_buffer, current_transfer_cmd_buffer);

		return;
	}

    void WindowRenderer::EndRenderWindow()
    {
		vk::Fence in_flight_fence = swapchain_->GetCurrentInFlightFence();

		vk::Semaphore render_finished_semaphore = m_render_device->EndFrame(m_current_image_available_semaphore, in_flight_fence);

		swapchain_->PresentCurrentImage(render_finished_semaphore);
    }

	void WindowRenderer::Recreate_Swapchain()
	{
		m_render_device->WaitIdle();

		swapchain_->Recreate_Swapchain();
	}

	void WindowRenderer::Reset()
	{
		// Destroy Windows Swapchain and its Resources
		{
		    swapchain_ = nullptr;
		    m_scene_renderer.reset();
		}

		m_graphics_pipeline->DestroyPipeline();

		m_render_device = nullptr;

		BRR_LogInfo("WindowRenderer Destroyed");
	}
}
