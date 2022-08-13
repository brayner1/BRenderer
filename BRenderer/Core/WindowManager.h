#ifndef BRR_WINDOWMANAGER_H
#define BRR_WINDOWMANAGER_H
#include "Window.h"
#include "Renderer/Renderer.h"

namespace brr
{
	class WindowManager
	{
	public:

		WindowManager(uint32_t width, uint32_t height);

		~WindowManager();

		void Update();

		void CloseWindow(uint32_t pWindowID);

		// Process a window event
		void ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent);

		[[nodiscard]] bool IsMainWindowClosed() const { return m_pMainWindowClosed; }

	private:
		uint32_t m_pMainWindowID = 0;
		std::unique_ptr<Window> m_pMainWindow{};
		bool m_pMainWindowClosed = true;

		// Here for posterior support for multiple windows
		std::unordered_map<uint32_t, uint32_t> m_pSecondaryWindows_Id_Index_Map{};
		std::vector<std::unique_ptr<Window>> m_pSecondaryWindows{};

		render::Renderer* m_pRenderer;
	};
}

#endif