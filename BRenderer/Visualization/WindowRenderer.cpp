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
    : m_owner_window(window),
      m_render_device(render::VKRD::GetSingleton())
    {
		m_swapchain = std::make_unique<render::Swapchain>(m_owner_window);

		Init_GraphicsPipeline();
    }

	WindowRenderer::~WindowRenderer()
	{
		if (m_render_device)
			m_render_device->WaitIdle();
		Destroy();
	}

	void WindowRenderer::Window_Resized()
	{
		Recreate_Swapchain();
	}

	void WindowRenderer::Init_GraphicsPipeline()
	{
        m_render_device->Create_GraphicsPipeline(m_swapchain.get());
	}

    void WindowRenderer::BeginRenderPass(vk::CommandBuffer cmd_buffer) const
    {
        std::array<vk::ClearValue, 2> clear_values { vk::ClearColorValue {0.2f, 0.2f, 0.2f, 1.f}, vk::ClearDepthStencilValue {1.0, 0} };

		glm::uvec2 extent = m_swapchain->GetSwapchainExtent();

        vk::RenderPassBeginInfo render_pass_begin_info{};
        render_pass_begin_info
            .setRenderPass(m_swapchain->GetRenderPass())
            .setFramebuffer(m_swapchain->GetFramebuffer(m_swapchain->GetCurrentImageIndex()))
            .setRenderArea(vk::Rect2D{ {0, 0},  {extent.x, extent.y}})
            .setClearValues(clear_values);

        cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	}

	void WindowRenderer::EndRenderPass(vk::CommandBuffer cmd_buffer) const
	{
        cmd_buffer.endRenderPass();
	}

    void WindowRenderer::Record_CommandBuffer(vk::CommandBuffer graphics_cmd_buffer)
	{
		SceneRenderer* scene_renderer = m_owner_window->GetScene()->GetSceneRenderer();
		scene_renderer->UpdateDirtyInstances();

		BeginRenderPass(graphics_cmd_buffer);

		scene_renderer->Render3D();

		EndRenderPass(graphics_cmd_buffer);
	}

    void WindowRenderer::BeginRenderWindow()
	{
		vk::Result result = m_swapchain->AcquireNextImage(m_current_image_available_semaphore);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			m_swapchain->Recreate_Swapchain();
			return;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire Swapchain image!");
		}

		m_frame_count = m_render_device->BeginFrame();

		SceneRenderer* scene_renderer = m_owner_window->GetScene()->GetSceneRenderer();
		scene_renderer->BeginRender();

        vk::CommandBuffer current_graphics_cmd_buffer = m_render_device->GetCurrentGraphicsCommandBuffer();

		Record_CommandBuffer(current_graphics_cmd_buffer);

		return;
	}

    void WindowRenderer::EndRenderWindow()
    {
		vk::Fence in_flight_fence = m_swapchain->GetCurrentInFlightFence();

		vk::Semaphore render_finished_semaphore = m_render_device->EndFrame(m_current_image_available_semaphore, in_flight_fence);

		m_swapchain->PresentCurrentImage(render_finished_semaphore);
    }

	void WindowRenderer::Recreate_Swapchain()
	{
		m_render_device->WaitIdle();

		m_swapchain->Recreate_Swapchain();
	}

    void WindowRenderer::Destroy()
    {
		// Destroy Windows Swapchain and its Resources
		{
		    m_swapchain = nullptr;
		}

		m_render_device = nullptr;

		BRR_LogInfo("WindowRenderer Destroyed");
    }
}
