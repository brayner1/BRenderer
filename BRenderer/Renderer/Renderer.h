#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include "Renderer/DevicePipeline.h"
#include "Renderer/Swapchain.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/Descriptors.h"

namespace brr{
    class WindowManager;
    class Scene;
	static constexpr int FRAME_LAG = 2;

	class Window;
	class PerspectiveCamera;
	namespace render
	{
        class SceneRenderer;
        class Shader;

        class Renderer
		{
		public:
			Renderer (Window* window, RenderDevice* device);

			~Renderer();

			void Window_Resized();

			vk::CommandBuffer BeginRenderWindow();
			void EndRenderWindow(vk::CommandBuffer cmd_buffer);

			/*
			 * Pipelines
			 */

			vk::Pipeline CreateGraphicsPipeline(const Shader& shader);

			[[nodiscard]] const DevicePipeline& GetGraphicsPipeline() const { return m_graphics_pipeline; }

			void Reset();

		private:
			friend class brr::WindowManager;

			// Define DescriptorSetLayout and create the GraphicsPipeline
			void Init_GraphicsPipeline();

			// Create UniformBuffers and the DescriptorSets
			void Init_DescriptorLayouts();

			// Initialize CommandBuffers, as well as define the basic synchronization primitives.
			void Init_CommandBuffers();

			void BeginRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index) const;
			void Record_CommandBuffer(vk::CommandBuffer cmd_buffer, vk::CommandBuffer present_cmd_buffer, uint32_t image_index);
			void EndRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer) const;

			void Recreate_Swapchain();

			struct UniformBufferObject
			{
				glm::mat4 projection_view{ 1.f };
			};

			// Descriptor Sets
			// TODO: Each window must have its own Descriptor Sets and Uniform Buffers? (If they use the same pipeline, I think not)
			// TODO: (Actually I think yes, because the pipeline defines only the layout, not the sets that we will use. Each resource may have its own combination of sets/buffers)
			vk::DescriptorSetLayout m_pDescriptorSetLayout {};

			// DevicePipeline

			DevicePipeline m_graphics_pipeline {};

			// Commands

			vk::CommandPool m_pCommandPool {};
			vk::CommandPool m_pPresentCommandPool {};
			vk::CommandPool m_pTransferCommandPool {};

			// Window
			Window* m_pOwnerWindow {};

			// Swapchain
			std::unique_ptr<Swapchain> swapchain_{};

			uint32_t current_image_idx{};

			// Command Buffers
			std::vector<vk::CommandBuffer> m_pCommandBuffers{};
			vk::CommandBuffer m_pPresentCommandBuffer{};

			std::unique_ptr<SceneRenderer> scene_renderer;

			RenderDevice* render_device_ {};
		};
	}
}

#endif