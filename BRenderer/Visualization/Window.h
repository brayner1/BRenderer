#ifndef BRR_WINDOW_H
#define BRR_WINDOW_H
#include <Scene/Scene.h>

namespace brr::vis
{
	typedef Uint32 WindowId;
	class Window
	{
	public:

		Window(const std::string& window_name, glm::uvec2 window_size);

		~Window();

		// Initialize and open the window with the current properties.
		void InitWindowRenderer();

		// Close the window if it is open. If it is already closed, then do nothing.
		void CloseWindow();

		// Process a window event
		void ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent);

		// Window API

		[[nodiscard]] constexpr WindowId GetWindowID() const { return m_window_id; }
		[[nodiscard]] constexpr SDL_Window* GetSDLWindowHandle() const { return m_window; }
		[[nodiscard]] constexpr bool NeedToClose() const { return m_need_to_close; }

		void RenderWindow();

		// Vulkan

		void GetRequiredVulkanExtensions(std::vector<const char*>& extensions) const;
		[[nodiscard]] glm::ivec2 GetWindowExtent() const;

		Scene* GetScene() const { return m_scene; }
		void SetScene(Scene* scene);

    private:
		WindowId m_window_id = 0;
		SDL_Window* m_window = nullptr;

		std::unique_ptr<WindowRenderer> m_window_renderer;

		Scene* m_scene = nullptr;

		bool m_need_to_close = false;
		bool m_minimized = false;
	};
}

#endif