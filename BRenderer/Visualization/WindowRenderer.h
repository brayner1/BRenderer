#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include <Core/thirdpartiesInc.h>
#include <Renderer/DeviceSwapchain.h>
#include <Renderer/Descriptors.h>

#include "SceneRenderer.h"

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

            void RenderWindow();

			void SetSceneRenderer(SceneRenderer* scene_renderer);

		private:
			friend class WindowManager;

			void Record_CommandBuffer(SceneRenderer* scene_renderer);

			void Recreate_Swapchain();

			void Destroy();

			// Window
			Window* m_owner_window{};

			ViewportId m_viewport = ViewportId::NULL_ID;

			SceneRenderer* m_scene_renderer;

			// DeviceSwapchain
			std::unique_ptr<render::DeviceSwapchain> m_swapchain{};
			std::vector<render::Texture2DHandle> m_swapchain_images;
			uint32_t m_swapchain_current_image_idx;

            vk::Semaphore m_current_image_available_semaphore {};

            render::VulkanRenderDevice* m_render_device {};
		};
	}
}

#endif