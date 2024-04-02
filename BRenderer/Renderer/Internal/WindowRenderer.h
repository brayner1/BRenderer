#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include <Core/thirdpartiesInc.h>
#include <Renderer/DeviceSwapchain.h>
#include <Renderer/SceneRenderer.h>

namespace brr{
    class Scene;
	
	class PerspectiveCamera;

	namespace vis
	{
	    class Window;
	}

	namespace render
	{
		class VulkanRenderDevice;
		class Shader;

        class WindowRenderer
		{
		public:
            WindowRenderer(uint32_t window_id,
                           glm::uvec2 window_extent,
                           SwapchainWindowHandle swapchain_window_handle);

			~WindowRenderer();

			void Device_NotifySurfaceLost() const;

			void Window_Resized(glm::uvec2 new_extent);

			void Window_SurfaceLost(glm::uvec2 window_extent,
                                    SwapchainWindowHandle swapchain_window_handle);

            void RenderWindow();

			void SetSceneRenderer(SceneRenderer* scene_renderer);

			uint32_t GetWindowId() const { return m_window_id; }

		private:

			void Record_CommandBuffer(SceneRenderer* scene_renderer);

			void Recreate_Swapchain();

			void Destroy();

			uint32_t m_window_id;

			glm::uvec2 m_window_extent {};

			ViewportId m_viewport = ViewportId::NULL_ID;

			SceneRenderer* m_scene_renderer;

			// DeviceSwapchain
			std::unique_ptr<DeviceSwapchain> m_swapchain{};
			std::vector<Texture2DHandle> m_swapchain_images;
			uint32_t m_swapchain_current_image_idx;

            VulkanRenderDevice* m_render_device {};
		};
	}
}

#endif