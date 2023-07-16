#include "WindowManager.h"

#include "Core/LogSystem.h"

namespace brr
{
	WindowManager::WindowManager(uint32_t width, uint32_t height)
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			BRR_LogInfo("Could not initialize SDL: {}", SDL_GetError());
			exit(1);
		}
		BRR_LogInfo("SDL Initialized.");

		m_pMainWindow = std::make_unique<Window>("Vulkan Test", glm::uvec2{600, 600});
		m_pMainWindowID = m_pMainWindow->GetWindowID();
		m_pMainWindowClosed = false;

		render::VKRD::CreateRenderDevice(m_pMainWindow.get());

		m_pMainWindow->InitWindowRenderer();

		BRR_LogInfo("WindowManager initialized.");
	}

	WindowManager::~WindowManager()
	{
		m_pSecondaryWindows_Id_Index_Map.clear();
		m_pSecondaryWindows.clear();

		m_pMainWindow.reset();

		SDL_Quit();
	}

	void WindowManager::Update()
	{
		if (!m_pMainWindowClosed)
		{
			m_pMainWindow->RenderWindow();
		}
	}

	void WindowManager::CloseWindow(WindowId pWindowID)
	{
		// If closing main window, close the application
		if (pWindowID == m_pMainWindowID)
		{
			//m_pRenderer->Destroy_Window(m_pMainWindow.get());
			m_pMainWindow->CloseWindow();
			m_pMainWindowClosed = true;
			return;
		}

		auto it = m_pSecondaryWindows_Id_Index_Map.find(pWindowID);
		if (it == m_pSecondaryWindows_Id_Index_Map.end())
			return;

		const WindowId window_index = it->second;
		m_pSecondaryWindows[window_index]->CloseWindow();

		m_pSecondaryWindows_Id_Index_Map.erase(pWindowID);
		m_pSecondaryWindows.erase(m_pSecondaryWindows.begin() + window_index);
	}

	void WindowManager::ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent)
	{
		Window* window;
		if (pWindowEvent.windowID == m_pMainWindowID)
		{
			window = m_pMainWindow.get();
		}
		else
		{
			const uint32_t window_index = m_pSecondaryWindows_Id_Index_Map[pWindowEvent.windowID];
			window = m_pSecondaryWindows[window_index].get();
		}

		window->ProcessWindowEvent(pWindowEvent);
		if (window->NeedToClose())
		{
			CloseWindow(pWindowEvent.windowID);
		}
	}
}
