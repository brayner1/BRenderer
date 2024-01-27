#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include <Core/thirdpartiesInc.h>
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

            void RenderWindow(SceneRenderer* scene_renderer);

		private:
			friend class WindowManager;

			void Record_CommandBuffer(SceneRenderer* scene_renderer);

			void Recreate_Swapchain();

			void Destroy();

			// Window
			Window* m_owner_window{};

			// Swapchain
			std::unique_ptr<render::Swapchain> m_swapchain{};
			size_t m_frame_count = 0;

            vk::Semaphore m_current_image_available_semaphore {};

            render::VulkanRenderDevice* m_render_device {};
		};
	}
}

#endif