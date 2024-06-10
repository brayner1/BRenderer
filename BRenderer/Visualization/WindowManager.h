#ifndef BRR_WINDOWMANAGER_H
#define BRR_WINDOWMANAGER_H
#include <Visualization/Window.h>

namespace brr::render
{
    class VulkanRenderDevice;
}

namespace brr::vis
{
	class WindowManager
	{
	public:

		static WindowManager* Instance();

		static void InitWindowManager(uint32_t width, uint32_t height);
		static void DestroyWindowManager();

		~WindowManager();

		void CloseWindow(WindowID pWindowID);

		void CloseMainWindow();

		// Process a window event
		void ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent);

		[[nodiscard]] bool IsMainWindowClosed() const { return m_main_window_closed; }

		WindowID GetMainWindowID() const { return m_main_window_ID; }
		Window* GetMainWindow() const { return m_main_window.get(); }

	private:

		WindowManager(uint32_t width, uint32_t height);

	private:
		WindowID m_main_window_ID = 0;
		std::unique_ptr<Window> m_main_window{};
		bool m_main_window_closed = true;

		//TODO: Here for posterior support for multiple windows
		std::unordered_map<WindowID, uint32_t> m_secondaryWindowsID_index_map{};
		std::vector<std::unique_ptr<Window>> m_secondary_windows{};

		render::VulkanRenderDevice* m_render_device {};
	};
}

#endif