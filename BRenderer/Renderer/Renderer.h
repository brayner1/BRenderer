#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include "Renderer/Swapchain.h"
#include "Renderer/RenderDevice.h"
#include "Geometry/Mesh2D.h"
#include "Renderer/VkInitializerHelper.h"

namespace brr{
	static constexpr int FRAME_LAG = 2;

	class Window;
	class PerspectiveCamera;
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

			/*
			 * Buffers
			 */

			void Create_Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage, 
				vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& buffer_memory);

			void Copy_Buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size, 
				vk::DeviceSize src_buffer_offset = 0, vk::DeviceSize dst_buffer_offset = 0);

			[[nodiscard]] vk::Device Get_VkDevice() const { return render_device_->Get_VkDevice(); }
			[[nodiscard]] uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties) const;

			void Reset();

		private:
			// NOTE: Function is private for now since multiple windows is not yet supported
			// TODO: Add multi-window support and make 'Create_Window' public.
			void Create_Window(Window* window);

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

			void Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index);

			void Recreate_Swapchain(RendererWindow& window);

			void Update_UniformBuffers(RendererWindow& window);


			static std::unique_ptr<Renderer> singleton;

			struct RendererWindow
			{
				Window* m_associated_window {};

				// Swapchain
				std::unique_ptr<Swapchain> swapchain_{};

				// Command Buffers
				std::vector<vk::CommandBuffer> m_pCommandBuffers{};
				vk::CommandBuffer m_pPresentCommandBuffer{};
			};

			struct UniformBufferObject
			{
				glm::mat4 projection_view{ 1.f };
			};

			// Windows
			static constexpr size_t MAIN_WINDOW_ID = 0;
			std::vector<RendererWindow> m_pWindows {};
			uint32_t m_pWindow_number = 0;
			std::unordered_map<uint32_t, uint32_t> m_pWindowId_index_map {};

			// Descriptor Sets
			// TODO: Each window must have its own Descriptor Sets and Uniform Buffers? (If they use the same pipeline, I think not)
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

			std::unique_ptr<RenderDevice> render_device_ {};

			PerspectiveCamera* camera { nullptr };

			Mesh2D* mesh {nullptr};
		};
	}
}

#endif