#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include "Geometry/Mesh2D.h"
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

			void Window_Resized(Window* window);

			void Destroy_Window(Window* window);

			void Draw();

			void Create_Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& buffer_memory);
			void Copy_Buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size);

			[[nodiscard]] vk::Device Get_VkDevice() const { return m_pDevice; }
			[[nodiscard]] uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties) const;

			void Reset();

		private:
			// NOTE: Function is private for now since multiple windows is not yet supported
			// TODO: Add multi-window support and make 'Create_Window' public.
			void Create_Window(Window* window);

			void Init_VkInstance(Window* window);
			void Init_Surface(RendererWindow& window);
			void Init_PhysDevice(vk::SurfaceKHR surface);
			void Init_Queues_Indices(vk::SurfaceKHR surface);
			void Init_Device();

			// Swapchain, renderpass and resources (Images, ImageViews and FrameBuffers
			void Init_Swapchain(RendererWindow& window);
			void Init_RenderPass(RendererWindow& window);
			void Init_Framebuffers(RendererWindow& window);

			// Define DescriptorSetLayout and create the GraphicsPipeline
			void Init_DescriptorSetLayout();
			void Init_GraphicsPipeline(RendererWindow& window);

			// Create UniformBuffers and the DescriptorSets
			void Init_UniformBuffers();
			void Init_DescriptorPool();
			void Init_DescriptorSets();

			// Initialize CommandPool and allocate CommandBuffers, as well as define the basic synchronization primitives.
			void Init_CommandPool();
			void Init_CommandBuffers(RendererWindow& window);
			void Init_Sychronization(RendererWindow& window);


			void Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index);

			void Recreate_Swapchain(RendererWindow& window);
			void Cleanup_Swapchain(RendererWindow& window);

			void Update_UniformBuffers(RendererWindow& window);


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

				// Swapchain
				vk::SwapchainKHR m_swapchain {};
				vk::SurfaceKHR m_surface {};
				std::vector<ImageResources> m_image_resources {};

				// Render Pass
				vk::RenderPass m_render_pass {};

				// Command Buffers
				std::vector<vk::CommandBuffer> m_pCommandBuffers{};
				vk::CommandBuffer m_pPresentCommandBuffer{};

				// Sync
				vk::Semaphore m_image_available_semaphores[FRAME_LAG];
				vk::Semaphore m_render_finished_semaphores[FRAME_LAG];
				vk::Fence m_in_flight_fences[FRAME_LAG];
				int current_buffer = 0;
			};

			struct UniformBufferObject
			{
				glm::mat4 projection_view{ 1.f };
			};

			vk::Instance m_pVkInstance {};

			// Physical and Logical Devices

			vk::PhysicalDevice m_pPhysDevice {};
			vk::Device m_pDevice {};

			// Windows
			static constexpr size_t MAIN_WINDOW_ID = 0;
			std::vector<RendererWindow> m_pWindows {};
			uint32_t m_pWindow_number = 0;
			std::unordered_map<uint32_t, uint32_t> m_pWindowId_index_map {};

			// Queues

			VkHelpers::QueueFamilyIndices m_pQueueFamilyIdx{};
			vk::Queue m_pGraphicsQueue {};
			vk::Queue m_pPresentationQueue {};
			vk::Queue m_pTransferQueue {};
			bool m_pQueues_initialized = false;
			bool m_pDifferentPresentQueue = false;
			bool m_pDifferentTransferQueue = false;

			// Swapchain

			vk::Format m_pSwapchain_ImageFormat {};
			vk::Extent2D m_pSwapchainExtent {};

			// Descriptor Sets
			// TODO: Each window must have its own Descriptor Sets and Uniform Buffers
			vk::DescriptorSetLayout m_pDescriptorSetLayout {};
			vk::DescriptorPool m_pDescriptorPool {};
			std::vector<vk::DescriptorSet> m_pDescriptorSets {};

			// Pipeline

			vk::PipelineLayout m_pPipelineLayout {};
			vk::Pipeline m_pGraphicsPipeline {};

			// Uniforms

			std::vector<DeviceBuffer> uniform_buffers_ {};

			// Commands

			vk::CommandPool m_pCommandPool {};
			vk::CommandPool m_pPresentCommandPool {};
			vk::CommandPool m_pTransferCommandPool {};

			Mesh2D* mesh {nullptr};
		};
	}
}

#endif