#ifndef BRR_RENDERER_H
#define BRR_RENDERER_H
#include <Core/thirdpartiesInc.h>
#include <Renderer/GpuResources/DeviceSwapchain.h>
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

            /**
			 * Notify MainThread that window render surface was lost.
			 * MainThread should queue a SurfaceOutdated command for this window.
			 */
			void Device_NotifySurfaceLost() const;

            /**
			 * Window was resized to a new extent.
			 * Will recreate Swapchain using the old Swapchain as base.
			 * @param new_extent New window extent.
			 */
			void Window_Resized(glm::uvec2 new_extent);

            /**
			 * Recreate new Swapchain using new window render surface when the previous render surface of this Window was lost.
			 * @param window_extent Extent of new Window surface.
			 * @param swapchain_window_handle Updated render surface for this window.
			 */
            void Window_SurfaceLost(glm::uvec2 window_extent,
                                    SwapchainWindowHandle swapchain_window_handle);

            /**
             * Render this window on the screen.
             */
            void RenderWindow(ImDrawData* imgui_draw_data);


            /**
			 * Get this window's scene renderer.
			 * @return scene renderer associated with this window.
			 */
			SceneRenderer* GetSceneRenderer() const { return m_scene_renderer; }

            /**
			 * Set the SceneRenderer and the Camera that will be used to render this window.
			 * @param scene_renderer The window's new scene renderer.
			 * @param camera_id The Scene's CameraID used to render this window.
			 */
			void SetSceneRenderer(SceneRenderer* scene_renderer, CameraID camera_id);

            /**
			 * @return The WindowID of the Window associated with this WindowRenderer.
			 */
			uint32_t GetWindowId() const { return m_window_id; }

		private:

            /**
			 * Record command buffer for the SceneRenderer of this window.
			 */
			void Record_CommandBuffer();

            /**
			 * Recreate the swapchain of this WindowRenderer using the old Swapchain as base.
			 */
			void Recreate_Swapchain();

			uint32_t m_window_id;

			glm::uvec2 m_window_extent {};

			ViewportID m_viewport = ViewportID::NULL_ID;

			SceneRenderer* m_scene_renderer = nullptr;

			// DeviceSwapchain
			std::unique_ptr<DeviceSwapchain> m_swapchain{};
			std::vector<Texture2DHandle> m_swapchain_images;
			uint32_t m_swapchain_current_image_idx;

            VulkanRenderDevice* m_render_device {};

			bool m_minimized = false;
		};
	}
}

#endif