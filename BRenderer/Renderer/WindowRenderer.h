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

            void BeginRenderWindow();
			void EndRenderWindow();

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

			void BeginCommandBuffer(vk::CommandBuffer cmd_buffer) const;
			void BeginRenderPass(vk::CommandBuffer cmd_buffer) const;
			void Record_CommandBuffer(vk::CommandBuffer graphics_cmd_buffer, vk::CommandBuffer transfer_cmd_buffer);
			void EndRenderPass(vk::CommandBuffer cmd_buffer) const;
			void EndCommandBuffer(vk::CommandBuffer cmd_buffer) const;

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
			size_t m_frame_count = 0;

			//uint32_t current_image_idx{};

			// Command Buffers
			std::array<vk::CommandBuffer, FRAME_LAG> m_pGraphicsCommandBuffers;
			std::array<vk::CommandBuffer, FRAME_LAG> m_pTransferCommandBuffers;


            vk::Semaphore m_current_image_available_semaphore {};
			vk::Semaphore render_finished_semaphores_[FRAME_LAG];
			vk::Semaphore transfer_finished_semaphores_[FRAME_LAG];

			std::unique_ptr<SceneRenderer> scene_renderer;

			VulkanRenderDevice* render_device_ {};
		};
	}
}

#endif