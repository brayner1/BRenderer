#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include "Renderer/DevicePipeline.h"
#include "Renderer/Swapchain.h"
#include "Renderer/RenderDevice.h"
#include "Geometry/Mesh2D.h"
#include "Renderer/Descriptors.h"

namespace brr{
    class WindowManager;
    class Scene;
	static constexpr int FRAME_LAG = 2;

	class Window;
	class PerspectiveCamera;
	namespace render
	{
        class Shader;

        class Renderer
		{
			struct ImageResources;
			struct RendererWindow;
		public:
			~Renderer();

			static Renderer* GetRenderer()
			{
				return singleton.get();
			}

			void Window_Resized(Window* window);

			void Destroy_Window(Window* window);

			void Draw(Window* window);

			/*
			 * Pipelines
			 */

			vk::Pipeline CreateGraphicsPipeline(const Shader& shader);

			/*
			 * Buffers
			 */

			void Create_Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage,
				vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& buffer_memory);

			void Copy_Buffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, vk::DeviceSize size,
				vk::DeviceSize src_buffer_offset = 0, vk::DeviceSize dst_buffer_offset = 0);

			[[nodiscard]] RenderDevice* GetDevice() const { return render_device_.get(); }
			[[nodiscard]] uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties) const;

			[[nodiscard]] DescriptorLayoutBuilder GetDescriptorLayoutBuilder() const;
			[[nodiscard]] DescriptorSetBuilder<FRAME_LAG> GetDescriptorSetBuilder(const DescriptorLayout& layout) const;

			[[nodiscard]] const DevicePipeline& GetGraphicsPipeline() const { return m_graphics_pipeline; }

			void Reset();

		private:
			friend class brr::WindowManager;
			Renderer();

			void Init_VulkanRenderer(Window* main_window);

			// NOTE: Function is private for now since multiple windows is not yet supported
			// TODO: Add multi-window support and make 'Create_Window' public.
			void Create_Window(Window* window);

			// Define DescriptorSetLayout and create the GraphicsPipeline
			void Init_GraphicsPipeline(RendererWindow& window);

			// Create UniformBuffers and the DescriptorSets
			void Init_UniformBuffers();
			void Init_DescriptorSets();

			// Initialize CommandBuffers, as well as define the basic synchronization primitives.
			void Init_CommandBuffers(RendererWindow& window);

			void BeginRenderPass_CommandBuffer(RendererWindow& rend_window, vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index) const;
			void BindPipeline_CommandBuffer(RendererWindow& rend_window, const DevicePipeline& pipeline, vk::CommandBuffer cmd_buffer) const;
			void Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index, Scene* scene);
			void EndRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer) const;

			void Recreate_Swapchain(RendererWindow& window);

			void Update_UniformBuffers(RendererWindow& window, Scene& scene);


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
			// TODO: (Actually I think yes, because the pipeline defines only the layout, not the sets that we will use. Each resource may have its own combination of sets/buffers)
			vk::DescriptorSetLayout m_pDescriptorSetLayout {};
			std::vector<vk::DescriptorSet> m_pDescriptorSets {};

			// DevicePipeline

			DescriptorAllocator*	m_pDescriptorAllocator   = nullptr;
			DescriptorLayoutCache*	m_pDescriptorLayoutCache = nullptr;

			DevicePipeline m_graphics_pipeline {};

			// Uniforms

			std::vector<DeviceBuffer> m_pUniform_buffers {};

			// Commands

			vk::CommandPool m_pCommandPool {};
			vk::CommandPool m_pPresentCommandPool {};
			vk::CommandPool m_pTransferCommandPool {};

			std::unique_ptr<RenderDevice> render_device_ {};
		};
	}
}

#endif