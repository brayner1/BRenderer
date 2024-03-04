#ifndef BRR_WINDOWMANAGER_H
#define BRR_WINDOWMANAGER_H
#include <Visualization/Window.h>

namespace brr::vis
{
	class WindowManager
	{
	public:

		WindowManager(uint32_t width, uint32_t height);

		~WindowManager();

		void Update();

		void CloseWindow(WindowId pWindowID);

		// Process a window event
		void ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent);

		[[nodiscard]] bool IsMainWindowClosed() const { return m_main_window_closed; }

		WindowId GetMainWindowID() const { return m_main_window_ID; }
		Window* GetMainWindow() const { return m_main_window.get(); }

	private:
		WindowId m_main_window_ID = 0;
		std::unique_ptr<Window> m_main_window{};
		bool m_main_window_closed = true;

		//TODO: Here for posterior support for multiple windows
		std::unordered_map<WindowId, uint32_t> m_secondaryWindowsID_index_map{};
		std::vector<std::unique_ptr<Window>> m_secondary_windows{};

		render::VulkanRenderDevice* m_render_device {};
	};
}

#endif