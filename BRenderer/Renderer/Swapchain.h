#ifndef BRR_SWAPCHAIN_H
#define BRR_SWAPCHAIN_H

#include <Renderer/Vulkan/VulkanInc.h>
#include <Renderer/RenderDefs.h>

#include <Core/thirdpartiesInc.h>

namespace brr::vis
{
	class Window;
}

namespace brr::render
{

	class VulkanRenderDevice;

	class Swapchain
	{
	public:

		Swapchain(vis::Window* window);
		~Swapchain();

		vk::Result AcquireNextImage(vk::Semaphore& image_available_semaphore);
		vk::Result PresentCurrentImage(vk::Semaphore wait_semaphore);

		void Recreate_Swapchain();
		void Cleanup_Swapchain();

        [[nodiscard]] FORCEINLINE glm::uvec2 GetSwapchainExtent() const
        {
            return {m_swapchain_extent.width, m_swapchain_extent.height};
        }

		[[nodiscard]] FORCEINLINE vk::RenderPass GetRenderPass() const { return m_render_pass; }

		[[nodiscard]] FORCEINLINE uint32_t GetCurrentImageIndex() const { return m_current_image_idx; }

		[[nodiscard]] FORCEINLINE vk::Framebuffer GetFramebuffer(uint32_t image_index) const { return m_image_resources[image_index].m_framebuffer; }

		[[nodiscard]] FORCEINLINE vk::Fence GetCurrentInFlightFence() const { return m_in_flight_fences[m_current_buffer_idx]; }

	private:

		void Init_Swapchain(vis::Window* window);
		void Init_SwapchainResources();
		void Init_RenderPass();
		void Init_Framebuffers();
		void Init_Synchronization();

        vis::Window* m_window = nullptr;

		// Device

		VulkanRenderDevice* m_render_device = nullptr;

		// Swapchain

		struct ImageResources
		{
			vk::Image m_image {};
			vk::ImageView m_image_view {};
			vk::Framebuffer m_framebuffer {};
		};

		vk::SurfaceKHR m_surface {};
		vk::Format m_swapchain_image_format {};
		vk::Extent2D m_swapchain_extent {};

		vk::SwapchainKHR m_swapchain {};
		std::vector<ImageResources> m_image_resources {};

		// Render pass

		vk::RenderPass m_render_pass {};

		// Synchronization

		vk::Semaphore m_image_available_semaphores[FRAME_LAG];
		vk::Fence m_in_flight_fences[FRAME_LAG];

	    uint32_t m_current_buffer_idx = 0;
		uint32_t m_current_image_idx = 0;
	};

}

#endif