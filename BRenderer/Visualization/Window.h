#ifndef BRR_WINDOW_H
#define BRR_WINDOW_H
#include <Scene/Scene.h>

#include <Visualization/SceneView.h>
#include <Visualization/WindowImGuiLayer.h>

namespace brr::vis
{
	typedef Uint32 WindowID;
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

		// Render Window ImGui layer.
		void RenderImGuiLayer();

		std::shared_ptr<WindowImGuiLayer> GetImGuiLayer() const { return m_imgui_layer; }

		// Set the Window ImGui layer.
		void SetImGuiLayer(std::shared_ptr<WindowImGuiLayer> imgui_layer);

		// Get Window SceneView
		SceneView& GetSceneView() { return m_scene_view; }

		// Window API

		[[nodiscard]] constexpr WindowID GetWindowID() const { return m_window_id; }
		[[nodiscard]] constexpr SDL_Window* GetSDLWindowHandle() const { return m_window; }
		[[nodiscard]] constexpr bool NeedToClose() const { return m_need_to_close; }

		[[nodiscard]] glm::ivec2 GetWindowExtent() const;

    private:
		WindowID m_window_id = 0;
		SDL_Window* m_window = nullptr;

		std::shared_ptr<WindowImGuiLayer> m_imgui_layer {};

		SceneView m_scene_view;

		bool m_need_to_close = false;
		bool m_minimized = false;
	};
}

#endif