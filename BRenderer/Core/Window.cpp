#include "Core/Window.h"

namespace brr
{
	uint32_t Window::InitWindow()
	{
		constexpr uint32_t SCREEN_WIDTH = 600;
		constexpr uint32_t SCREEN_HEIGHT = 600;

		const uint32_t window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN;
		m_pWindow = SDL_CreateWindow("Vulkan Test", 
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			SCREEN_WIDTH, SCREEN_HEIGHT, 
			window_flags);
		
		if (!m_pWindow)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open %d x %d window: %s", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
			exit(1);
		}

		uint32_t winId = SDL_GetWindowID(m_pWindow);
		if (!winId)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get window ID: %s", SDL_GetError());
			exit(1);
		}

		m_pWindowID = winId;

		SDL_Log("Window %d Created", winId);

		return m_pWindowID;
	}

	void Window::CloseWindow()
	{
		if (m_pWindow)
		{
			SDL_DestroyWindow(m_pWindow);
			m_pWindowID = 0;
		}

	}

	Window::~Window()
	{
		CloseWindow();
		SDL_Log("Window Closed");
	}

	void Window::ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent)
	{
		switch ((SDL_WindowEventID)pWindowEvent.event)
		{
			case SDL_WINDOWEVENT_SHOWN: break;
			case SDL_WINDOWEVENT_HIDDEN: break;
			case SDL_WINDOWEVENT_EXPOSED: break;
			case SDL_WINDOWEVENT_MOVED: break;
			case SDL_WINDOWEVENT_RESIZED: break;
			case SDL_WINDOWEVENT_SIZE_CHANGED: break;
			case SDL_WINDOWEVENT_MINIMIZED: break;
			case SDL_WINDOWEVENT_MAXIMIZED: break;
			case SDL_WINDOWEVENT_RESTORED: break;
			case SDL_WINDOWEVENT_ENTER: break;
			case SDL_WINDOWEVENT_LEAVE: break;
			case SDL_WINDOWEVENT_FOCUS_GAINED: break;
			case SDL_WINDOWEVENT_FOCUS_LOST: break;
			case SDL_WINDOWEVENT_CLOSE:
			{
				SDL_DestroyWindow(m_pWindow);
				m_pWindow = nullptr;
				m_pWindowID = 0;
				break;
			}
			case SDL_WINDOWEVENT_TAKE_FOCUS: break;
			case SDL_WINDOWEVENT_HIT_TEST: break;
			case SDL_WINDOWEVENT_ICCPROF_CHANGED: break;
			case SDL_WINDOWEVENT_DISPLAY_CHANGED: break;
			default: ;
		}
	}


}
