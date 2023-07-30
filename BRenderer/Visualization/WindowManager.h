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

		[[nodiscard]] bool IsMainWindowClosed() const { return m_pMainWindowClosed; }

		WindowId GetMainWindowID() const { return m_pMainWindowID; }
		Window* GetMainWindow() const { return m_pMainWindow.get(); }

	private:
		WindowId m_pMainWindowID = 0;
		std::unique_ptr<Window> m_pMainWindow{};
		bool m_pMainWindowClosed = true;

		//TODO: Here for posterior support for multiple windows
		std::unordered_map<WindowId, uint32_t> m_pSecondaryWindows_Id_Index_Map{};
		std::vector<std::unique_ptr<Window>> m_pSecondaryWindows{};
	};
}

#endif