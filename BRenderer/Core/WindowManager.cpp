#include "WindowManager.h"

namespace brr
{
	WindowManager::WindowManager(uint32_t width, uint32_t height)
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cout << "Could not initialize SDL: " << SDL_GetError() << std::endl;
			exit(1);
		}

		m_pMainWindow = std::make_unique<Window>();
		m_pMainWindow->InitWindow();
		m_pMainWindowID = m_pMainWindow->GetWindowID();
		m_pMainWindowClosed = false;

		m_pRenderer = new render::Renderer();
		m_pRenderer->Init_VulkanRenderer(m_pMainWindow.get());
	}

	WindowManager::~WindowManager()
	{
		//m_pRenderer->Reset();
		//delete m_pRenderer;

		m_pSecondaryWindows_Id_Index_Map.clear();
		m_pSecondaryWindows.clear();

		//m_pMainWindow.reset();

		SDL_Quit();
	}

	void WindowManager::Update()
	{
		if (!m_pMainWindowClosed)
			m_pRenderer->Draw(m_pMainWindow.get());
	}

	void WindowManager::CloseWindow(WindowId pWindowID)
	{
		// If closing main window, close the application
		if (pWindowID == m_pMainWindowID)
		{
			m_pRenderer->Destroy_Window(m_pMainWindow.get());
			m_pMainWindow->CloseWindow();
			m_pMainWindowClosed = true;
			return;
		}

		auto it = m_pSecondaryWindows_Id_Index_Map.find(pWindowID);
		if (it == m_pSecondaryWindows_Id_Index_Map.end())
			return;

		const WindowId window_index = it->second;
		m_pRenderer->Destroy_Window(m_pSecondaryWindows[window_index].get());
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
