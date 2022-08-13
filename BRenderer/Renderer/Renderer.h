#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include "Renderer/VkInitializerHelper.h"

namespace brr{
	static constexpr int FRAME_LAG = 2;

	class Window;
	namespace render
	{
		class Renderer
		{
			struct ImageResources;
			struct RendererWindow;
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

			void Init_VulkanRenderer(Window* main_window);
			

			void Destroy_Window(Window* window);

			void Draw();

			[[nodiscard]] vk::Device Get_VkDevice() const { return m_pDevice; }

			void Reset();

		private:
			// NOTE: Function is private for now since multiple windows is not yet supported
			// TODO: Add multi-window support and make 'Create_Window' public.
			void Create_Window(Window* window);

			void Init_VkInstance(Window* window);
			void Init_Surface(RendererWindow& window);
			void Init_Queues(vk::SurfaceKHR surface);
			void Init_PhysDevice(vk::SurfaceKHR surface);
			void Init_Device();
			void Init_Swapchain(RendererWindow& window);
			void Init_RenderPass(RendererWindow& window);
			void Init_GraphicsPipeline(RendererWindow& window);
			void Init_Framebuffers(RendererWindow& window);
			void Init_CommandPool();
			void Init_CommandBuffers(RendererWindow& window);
			void Init_Sychronization(RendererWindow& window);
			void Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index);


			static std::unique_ptr<Renderer> singleton;

			struct ImageResources
			{
				vk::Image m_image;
				vk::ImageView m_image_view;
				vk::Framebuffer m_framebuffer;
			};

			struct RendererWindow
			{
				Window* m_associated_window {};
				vk::SurfaceKHR m_surface {};
				vk::SwapchainKHR m_swapchain {};
				std::vector<ImageResources> m_image_resources {};
				vk::RenderPass m_render_pass {};

				std::vector<vk::CommandBuffer> m_pCommandBuffers{};
				vk::CommandBuffer m_pPresentCommandBuffer{};

				// Sync
				vk::Semaphore m_image_available_semaphores[FRAME_LAG];
				vk::Semaphore m_render_finished_semaphores[FRAME_LAG];
				vk::Fence m_in_flight_fences[FRAME_LAG];
				int current_buffer = 0;
			};

			vk::Instance m_pVkInstance {};

			// Physical and Logical Devices

			vk::PhysicalDevice m_pPhysDevice {};
			vk::Device m_pDevice {};

			// Windows
			static constexpr size_t MAIN_WINDOW_ID = 0;
			std::vector<RendererWindow> m_pWindows {};
			uint32_t m_pWindow_number = 0;

			// Queues

			VkHelpers::QueueFamilyIndices m_pQueueFamilyIdx{};
			vk::Queue m_pGraphicsQueue {};
			vk::Queue m_pPresentationQueue {};
			bool m_pQueues_initialized = false;
			bool m_pDifferentPresentQueue = false;

			// Swapchain

			vk::Format m_pSwapchain_ImageFormat {};
			vk::Extent2D m_pSwapchainExtent {};

			// Pipeline

			vk::PipelineLayout m_pPipelineLayout {};
			vk::Pipeline m_pGraphicsPipeline {};

			// Commands

			vk::CommandPool m_pCommandPool {};
			vk::CommandPool m_pPresentCommandPool{};

			/*vk::CommandBuffer m_pCommandBuffer {};
			vk::CommandBuffer m_pPresentCommandBuffer {};*/
		};
	}
}

#endif