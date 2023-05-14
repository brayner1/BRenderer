#ifndef BRR_WINDOW_H
#define BRR_WINDOW_H
#include "Scene/Scene.h"

namespace brr
{
	typedef Uint32 WindowId;
	class Window
	{
	public:

		Window(const std::string& window_name, glm::uvec2 window_size);

		~Window();

		// Initialize and open the window with the current properties.
		void InitWindowRenderer(render::RenderDevice* render_device);

		// Close the window if it is open. If it is already closed, then do nothing.
		void CloseWindow();

		// Process a window event
		void ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent);

		// Window API

		[[nodiscard]] WindowId GetWindowID() const { return m_pWindowID; }
		[[nodiscard]] SDL_Window* GetSDLWindowHandle() const { return m_pWindow; }
		[[nodiscard]] bool NeedToClose() const { return m_pNeedToClose; }

		void RenderWindow();

		// Vulkan

		void GetRequiredVulkanExtensions(std::vector<const char*>& extensions) const;
		[[nodiscard]] glm::ivec2 GetWindowExtent() const;
		[[nodiscard]] vk::SurfaceKHR GetVulkanSurface(vk::Instance instance);

		

		Scene* GetScene() const { return scene.get(); }

	private:
		bool m_pNeedToClose = false;
		WindowId m_pWindowID = 0;
		SDL_Window* m_pWindow = nullptr;

		vk::SurfaceKHR window_surface_ {};

		std::unique_ptr<render::WindowRenderer> m_pWindowRenderer;

		std::unique_ptr<Scene> scene;
	};
}

#endif