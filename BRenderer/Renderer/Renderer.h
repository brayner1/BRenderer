#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include "Renderer/VkInitializerHelper.h"

namespace brr::render
{
	class Renderer
	{
	public:
		Renderer();

		~Renderer();

		static Renderer* GetRenderer()
		{
			if (!singleton)
			{
				singleton = std::make_unique<Renderer>();
			}
			return singleton.get();
		}

		void Init_Renderer(SDL_Window* main_window);

		void Draw();

		[[nodiscard]] vk::Instance Get_VkInstance() const { return m_pVkInstance; }
		[[nodiscard]] vk::Device Get_VkDevice() const { return m_pDevice; }
		[[nodiscard]] vk::SurfaceKHR Get_VkSurface() const { return m_pSurface; }

		void Reset();

	private:
		void Init_VkInstance(SDL_Window* sdl_window);
		void Init_Surface(SDL_Window* sdl_window);
		void Init_PhysDevice();
		void Init_Device();
		void Init_Swapchain(SDL_Window* sdl_window);
		void Init_RenderPass();
		void Init_GraphicsPipeline();
		void Init_Framebuffers();
		void Init_CommandPool();
		void Init_CommandBuffer();
		void Init_Sychronization();
		void Record_CommandBuffer(vk::CommandBuffer cmd_buffer, uint32_t image_index);


		static std::unique_ptr<Renderer> singleton;

		struct ImageResources
		{
			vk::Image m_image;
			vk::ImageView m_image_view;
			vk::Framebuffer m_framebuffer;
		};

		vk::Instance m_pVkInstance {};

		// Surface
		vk::SurfaceKHR m_pSurface {};

		// Physical and Logical Devices

		vk::PhysicalDevice m_pPhysDevice {};
		vk::Device m_pDevice {};

		// Queues

		VkHelpers::QueueFamilyIndices m_pQueueFamilyIdx{};
		vk::Queue m_pGraphicsQueue {};
		vk::Queue m_pPresentationQueue {};
		bool m_pDifferentPresentQueue = false;

		// Swapchain

		vk::SwapchainKHR m_pSwapchain {};
		std::vector<ImageResources> m_pSwapchain_imageResources {};
		vk::Format m_pSwapchain_ImageFormat {};
		vk::Extent2D m_pSwapchainExtent {};

		// Pipeline

		vk::RenderPass m_pRenderPass {};
		vk::PipelineLayout m_pPipelineLayout {};
		vk::Pipeline m_pGraphicsPipeline {};

		// Commands

		vk::CommandPool m_pCommandPool {};
		vk::CommandPool m_pPresentCommandPool{};

		vk::CommandBuffer m_pCommandBuffer {};
		vk::CommandBuffer m_pPresentCommandBuffer {};

		// Synchronization

		vk::Semaphore m_pImageAvailableSemaphore {};
		vk::Semaphore m_pRenderFinishedSemaphore {};
		vk::Fence m_pInFlightFence {};
	};
}

#endif