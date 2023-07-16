#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include "Renderer/DevicePipeline.h"
#include "Renderer/Swapchain.h"
#include "Renderer/Descriptors.h"

namespace brr{
    class WindowManager;
    class Scene;

	class Window;
	class PerspectiveCamera;
	namespace render
	{
		class VulkanRenderDevice;
        class SceneRenderer;
        class Shader;

        class WindowRenderer
		{
		public:
			WindowRenderer (Window* window);

			~WindowRenderer();

			void Window_Resized();

			vk::CommandBuffer BeginRenderWindow();
			void EndRenderWindow(vk::CommandBuffer cmd_buffer);

			/*
			 * Pipelines
			 */

			vk::Pipeline CreateGraphicsPipeline(const Shader& shader);

			[[nodiscard]] const DevicePipeline* GetGraphicsPipeline() const { return m_graphics_pipeline.get(); }

			void Reset();

		private:
			friend class brr::WindowManager;

			// Define DescriptorSetLayout and create the GraphicsPipeline
			void Init_GraphicsPipeline();

			// Create UniformBuffers and the DescriptorSets
			void Init_DescriptorLayouts();

			// Initialize CommandBuffers, as well as define the basic synchronization primitives.
			void Init_CommandBuffers();

			void Init_Synchronization();

			void BeginRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer) const;
			void Record_CommandBuffer(vk::CommandBuffer cmd_buffer);
			void EndRenderPass_CommandBuffer(vk::CommandBuffer cmd_buffer) const;

			void Recreate_Swapchain();

			struct UniformBufferObject
			{
				glm::mat4 projection_view{ 1.f };
			};

			// Window
			Window* m_pOwnerWindow{};

			// Descriptor Sets
			vk::DescriptorSetLayout m_pDescriptorSetLayout {};

			// DevicePipeline

			std::unique_ptr<DevicePipeline> m_graphics_pipeline {};

			// Swapchain
			std::unique_ptr<Swapchain> swapchain_{};

			//uint32_t current_image_idx{};

			// Command Buffers
			std::vector<vk::CommandBuffer> m_pCommandBuffers {};

			vk::Semaphore m_current_image_available_semaphore {};
			vk::Semaphore render_finished_semaphores_[FRAME_LAG];

			std::unique_ptr<SceneRenderer> scene_renderer;

			VulkanRenderDevice* render_device_ {};
		};
	}
}

#endif