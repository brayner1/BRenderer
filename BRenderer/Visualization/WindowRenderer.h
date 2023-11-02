#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include <Core/thirdpartiesInc.h>
#include <Renderer/DevicePipeline.h>
#include <Renderer/Swapchain.h>
#include <Renderer/Descriptors.h>

namespace brr{
    class Scene;
	
	class PerspectiveCamera;

	namespace render
	{
		class VulkanRenderDevice;
		class Shader;
	}

	namespace vis
	{
		class Window;
		class WindowManager;
		class SceneRenderer;

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

			vk::Pipeline CreateGraphicsPipeline(const render::Shader& shader);

			[[nodiscard]] const render::DevicePipeline* GetGraphicsPipeline() const { return m_graphics_pipeline.get(); }

		private:
			friend class WindowManager;

			// Define DescriptorSetLayout and create the GraphicsPipeline
			void Init_GraphicsPipeline();

			// Create UniformBuffers and the DescriptorSets
			void Init_DescriptorLayouts();

			void BeginRenderPass(vk::CommandBuffer cmd_buffer) const;
			void Record_CommandBuffer(vk::CommandBuffer graphics_cmd_buffer);
			void EndRenderPass(vk::CommandBuffer cmd_buffer) const;

			void Recreate_Swapchain();

			void Destroy();

			// Window
			Window* m_owner_window{};

			// Descriptor Sets
			vk::DescriptorSetLayout m_descriptor_set_layout {};

			// DevicePipeline

			std::unique_ptr<render::DevicePipeline> m_graphics_pipeline {};

			// Swapchain
			std::unique_ptr<render::Swapchain> m_swapchain{};
			size_t m_frame_count = 0;

            vk::Semaphore m_current_image_available_semaphore {};

			std::unique_ptr<SceneRenderer> m_scene_renderer;

            render::VulkanRenderDevice* m_render_device {};
		};
	}
}

#endif