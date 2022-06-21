#include "Core/App.h"
#include "Core/Window.h"

namespace brr
{
	App::App()
	{
	}

	void App::Run()
	{
		Init();
		MainLoop();
		Clear();
	}

	void App::Init()
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cout << "Could not initialize SDL: " << SDL_GetError() << std::endl;
			exit(1);
		}

		m_pMainWindow = std::make_unique<Window>();
		m_pMainWindow->InitWindow();
		m_pMainWindowID = m_pMainWindow->GetWindowID();

		m_pRenderer = new render::Renderer();
		m_pRenderer->Init_Renderer(m_pMainWindow->GetSDLWindowHandle());
	}

	void App::MainLoop()
	{
		while(!m_pShouldFinish)
		{
			SDL_Event sdl_event;
			while (SDL_PollEvent(&sdl_event))
			{
				ProcessEvent(sdl_event);
			}

			m_pRenderer->Draw();

		}
	}

	void App::CloseWindow(uint32_t pWindowID)
	{
		// If closing main window, close the application
		if (pWindowID == m_pMainWindowID)
		{
			//m_pMainWindow->CloseWindow();
			m_pShouldFinish = true;
			return;
		}

		auto it = m_pSecondaryWindows_Id_Index_Map.find(pWindowID);
		if (it == m_pSecondaryWindows_Id_Index_Map.end())
			return;

		const uint32_t window_index = it->second;
		m_pSecondaryWindows[window_index]->CloseWindow();

		m_pSecondaryWindows_Id_Index_Map.erase(pWindowID);
		m_pSecondaryWindows.erase(m_pSecondaryWindows.begin() + window_index);
	}

	void App::Clear()
	{
		m_pRenderer->Reset();

		m_pSecondaryWindows_Id_Index_Map.clear();
		m_pSecondaryWindows.clear();

		m_pMainWindow.reset();

		SDL_Log("Renderer Destroyed");

		SDL_Quit();
	}

	void App::ProcessEvent(SDL_Event& pEvent)
	{
		switch ((SDL_EventType)pEvent.type)
		{
			// Last window closed, close application
			case SDL_QUIT:
			{
				m_pShouldFinish = true;
				break;
			}
			case SDL_APP_TERMINATING: break;
			case SDL_APP_LOWMEMORY: break;
			case SDL_APP_WILLENTERBACKGROUND: break;
			case SDL_APP_DIDENTERBACKGROUND: break;
			case SDL_APP_WILLENTERFOREGROUND: break;
			case SDL_APP_DIDENTERFOREGROUND: break;
			case SDL_LOCALECHANGED: break;
			case SDL_DISPLAYEVENT: break;
			case SDL_WINDOWEVENT:
			{
				Window* window;
				if (pEvent.window.windowID == m_pMainWindowID)
				{
					window = m_pMainWindow.get();
				}
				else
				{
					const uint32_t window_index = m_pSecondaryWindows_Id_Index_Map[pEvent.window.windowID];
					window = m_pSecondaryWindows[window_index].get();
				}
				window->ProcessWindowEvent(pEvent.window);
				if (window->WindowClosed())
				{
					CloseWindow(pEvent.window.windowID);
				}
				break;
			}
			case SDL_SYSWMEVENT: break; // This event is disabled by default. Encouraged to avoid if you can find less platform-specific way to accomplish your goals.
			case SDL_KEYDOWN: break;
			case SDL_KEYUP: break;
			case SDL_TEXTEDITING: break;
			case SDL_TEXTINPUT: break;
			case SDL_KEYMAPCHANGED: break;
			case SDL_MOUSEMOTION: break;
			case SDL_MOUSEBUTTONDOWN: break;
			case SDL_MOUSEBUTTONUP: break;
			case SDL_MOUSEWHEEL: break;
			case SDL_JOYAXISMOTION: break;
			case SDL_JOYBALLMOTION: break;
			case SDL_JOYHATMOTION: break;
			case SDL_JOYBUTTONDOWN: break;
			case SDL_JOYBUTTONUP: break;
			case SDL_JOYDEVICEADDED: break;
			case SDL_JOYDEVICEREMOVED: break;
			case SDL_CONTROLLERAXISMOTION: break;
			case SDL_CONTROLLERBUTTONDOWN: break;
			case SDL_CONTROLLERBUTTONUP: break;
			case SDL_CONTROLLERDEVICEADDED: break;
			case SDL_CONTROLLERDEVICEREMOVED: break;
			case SDL_CONTROLLERDEVICEREMAPPED: break;
			case SDL_CONTROLLERTOUCHPADDOWN: break;
			case SDL_CONTROLLERTOUCHPADMOTION: break;
			case SDL_CONTROLLERTOUCHPADUP: break;
			case SDL_CONTROLLERSENSORUPDATE: break;
			case SDL_FINGERDOWN: break;
			case SDL_FINGERUP: break;
			case SDL_FINGERMOTION: break;
			case SDL_DOLLARGESTURE: break;
			case SDL_DOLLARRECORD: break;
			case SDL_MULTIGESTURE: break;
			case SDL_CLIPBOARDUPDATE: break;
			case SDL_DROPFILE: break;
			case SDL_DROPTEXT: break;
			case SDL_DROPBEGIN: break;
			case SDL_DROPCOMPLETE: break;
			case SDL_AUDIODEVICEADDED: break;
			case SDL_AUDIODEVICEREMOVED: break;
			case SDL_SENSORUPDATE: break;
			case SDL_RENDER_TARGETS_RESET: break;
			case SDL_RENDER_DEVICE_RESET: break;
			case SDL_POLLSENTINEL: break;
			case SDL_USEREVENT: break;
			case SDL_LASTEVENT: break;
			default:;
		}
	}
}
