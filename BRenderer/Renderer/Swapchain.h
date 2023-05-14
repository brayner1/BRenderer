#ifndef BRR_SWAPCHAIN_H
#define BRR_SWAPCHAIN_H

#include "Renderer/RenderDefs.h"

namespace brr
{
	class Window;
}

namespace brr::render
{

	class RenderDevice;

	class Swapchain
	{
	public:

		Swapchain(RenderDevice* device, Window* window);
		~Swapchain();

		vk::Result AcquireNextImage(uint32_t& out_image_index);
		vk::Result SubmitCommandBuffer(vk::CommandBuffer command_buffer, uint32_t image_index);

		void Recreate_Swapchain();
		void Cleanup_Swapchain();

		FORCEINLINE [[nodiscard]] vk::SurfaceKHR GetSurface() const { return surface_; }

		FORCEINLINE [[nodiscard]] vk::Format GetSwapchain_Image_Format() const { return swapchain_image_format_; }

		FORCEINLINE [[nodiscard]] vk::Extent2D GetSwapchain_Extent() const { return swapchain_extent_; }

		FORCEINLINE [[nodiscard]] vk::SwapchainKHR GetSwapchain() const { return swapchain_; }

		FORCEINLINE [[nodiscard]] vk::RenderPass GetRender_Pass() const { return render_pass_; }

		FORCEINLINE [[nodiscard]] uint32_t GetCurrentBuffer() const { return current_buffer_; }

		FORCEINLINE [[nodiscard]] vk::Framebuffer GetFramebuffer(uint32_t image_index) const { return image_resources_[image_index].m_framebuffer; }



	private:

		void Init_Swapchain(Window* window);
		void Init_SwapchainResources();
		void Init_RenderPass();
		void Init_Framebuffers();
		void Init_Synchronization();

		

		Window* window_ = nullptr;

		// Device

		RenderDevice* device_ = nullptr;

		// Swapchain

		struct ImageResources
		{
			vk::Image m_image {};
			vk::ImageView m_image_view {};
			vk::Framebuffer m_framebuffer {};
		};

		vk::SurfaceKHR surface_ {};
		vk::Format swapchain_image_format_ {};
		vk::Extent2D swapchain_extent_ {};

		vk::SwapchainKHR swapchain_ {};
		std::vector<ImageResources> image_resources_ {};

		// Render pass

		vk::RenderPass render_pass_ {};

		// Synchronization

		vk::Semaphore image_available_semaphores_[FRAME_LAG];
		vk::Semaphore render_finished_semaphores_[FRAME_LAG];
		vk::Fence in_flight_fences_[FRAME_LAG];
		uint32_t current_buffer_ = 0;
	};

}

#endif