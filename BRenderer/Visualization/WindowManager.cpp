#include "WindowManager.h"

#include "Core/LogSystem.h"

namespace brr::vis
{
	WindowManager::WindowManager(uint32_t width, uint32_t height)
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			BRR_LogInfo("Could not initialize SDL: {}", SDL_GetError());
			exit(1);
		}
		BRR_LogInfo("SDL Initialized.");

		m_main_window = std::make_unique<Window>("Vulkan Test", glm::uvec2{600, 600});
		m_main_window_ID = m_main_window->GetWindowID();
		m_main_window_closed = false;

		render::VKRD::CreateRenderDevice(m_main_window.get());

		// Begin frame earlier to initialize command buffers and start transferring data to GPU.
		render::VKRD::GetSingleton()->BeginFrame(); 

		m_main_window->InitWindowRenderer();

		BRR_LogInfo("WindowManager initialized.");
	}

	WindowManager::~WindowManager()
	{
		m_secondaryWindowsID_index_map.clear();
		m_secondary_windows.clear();

		m_main_window.reset();

		render::VKRD::DestroyRenderDevice();

		SDL_Quit();
	}

	void WindowManager::Update()
	{
		if (!m_main_window_closed)
		{
			m_main_window->RenderWindow();
		}
	}

	void WindowManager::CloseWindow(WindowId pWindowID)
	{
		// If closing main window, close the application
		if (pWindowID == m_main_window_ID)
		{
			//m_pRenderer->Destroy_Window(m_main_window.get());
			m_main_window->CloseWindow();
			m_main_window_closed = true;
			return;
		}

		auto it = m_secondaryWindowsID_index_map.find(pWindowID);
		if (it == m_secondaryWindowsID_index_map.end())
			return;

		const WindowId window_index = it->second;
		m_secondary_windows[window_index]->CloseWindow();

		m_secondaryWindowsID_index_map.erase(pWindowID);
		m_secondary_windows.erase(m_secondary_windows.begin() + window_index);
	}

	void WindowManager::ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent)
	{
		Window* window;
		if (pWindowEvent.windowID == m_main_window_ID)
		{
			window = m_main_window.get();
		}
		else
		{
			const uint32_t window_index = m_secondaryWindowsID_index_map[pWindowEvent.windowID];
			window = m_secondary_windows[window_index].get();
		}

		window->ProcessWindowEvent(pWindowEvent);
		if (window->NeedToClose())
		{
			CloseWindow(pWindowEvent.windowID);
		}
	}
}
