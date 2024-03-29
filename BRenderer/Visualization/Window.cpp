#include "Window.h"

#include <Visualization/WindowRenderer.h>
#include <Core/LogSystem.h>

namespace brr::vis
{
	Window::Window(const std::string& window_name, glm::uvec2 window_size)
	{
		const uint32_t window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
		m_window = SDL_CreateWindow(window_name.c_str(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			window_size.x, window_size.y,
			window_flags);

		if (!m_window)
		{
			BRR_LogError("Failed to open {} x {} window: {}", window_size.x, window_size.y, SDL_GetError());
			exit(1);
		}

		uint32_t winId = SDL_GetWindowID(m_window);
		if (!winId)
		{
			BRR_LogError("Failed to get window ID: {}", SDL_GetError());
			exit(1);
		}

		m_window_id = winId;

		BRR_LogInfo("Window {} Created", winId);
		int width, height;
		SDL_Vulkan_GetDrawableSize(m_window, &width, &height);
		const glm::ivec2 extent{ width, height };
	}

	void Window::CloseWindow()
	{
		if (m_window)
		{
			m_window_renderer.reset();
			SDL_DestroyWindow(m_window);
			m_window = nullptr;
			m_window_id = 0;
		}
	}

	Window::~Window()
	{
		CloseWindow();
		BRR_LogInfo("Window Closed");
	}

    void Window::InitWindowRenderer()
    {
		m_window_renderer = std::make_unique<WindowRenderer>(this);
    }

    void Window::ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent)
	{
		switch ((SDL_WindowEventID)pWindowEvent.event)
		{
		case SDL_WINDOWEVENT_SHOWN: break;
		case SDL_WINDOWEVENT_HIDDEN: break;
		case SDL_WINDOWEVENT_EXPOSED: break;
		case SDL_WINDOWEVENT_MOVED: break;
		case SDL_WINDOWEVENT_RESIZED:
			BRR_LogInfo("Window {} resized.", m_window_id);
			m_window_renderer->Window_Resized();
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED: break;
		case SDL_WINDOWEVENT_MINIMIZED:
			BRR_LogInfo("Window {} minimized.", m_window_id);
			m_minimized = true;
			break;
		case SDL_WINDOWEVENT_MAXIMIZED: break;
		case SDL_WINDOWEVENT_RESTORED:
			BRR_LogInfo("Window {} restored.", m_window_id);
			m_minimized = false;
			break;
		case SDL_WINDOWEVENT_ENTER: break;
		case SDL_WINDOWEVENT_LEAVE: break;
		case SDL_WINDOWEVENT_FOCUS_GAINED: break;
		case SDL_WINDOWEVENT_FOCUS_LOST: break;
		case SDL_WINDOWEVENT_CLOSE:
		{
			BRR_LogInfo("Window {} closed.", m_window_id);
			m_need_to_close = true;
			break;
		}
		case SDL_WINDOWEVENT_TAKE_FOCUS: break;
		case SDL_WINDOWEVENT_HIT_TEST: break;
		case SDL_WINDOWEVENT_ICCPROF_CHANGED: break;
		case SDL_WINDOWEVENT_DISPLAY_CHANGED: break;
		default:;
		}
	}

    void Window::RenderWindow()
    {
		if (m_minimized)
			return;

		m_window_renderer->RenderWindow();
    }

	void Window::GetRequiredVulkanExtensions(std::vector<const char*>& extensions) const
	{
		unsigned int extension_count;

		// Get the number of extensions required by the SDL window.
		if (!SDL_Vulkan_GetInstanceExtensions(m_window,
			&extension_count, nullptr))
		{
			BRR_LogError("Could not get the number of extensions required by SDL: {}", SDL_GetError());
			exit(1);
		}

		const size_t additional_extension_count = extensions.size();
		extensions.resize(additional_extension_count + extension_count);

		if (!SDL_Vulkan_GetInstanceExtensions(m_window,
			&extension_count, extensions.data() + additional_extension_count))
		{
			BRR_LogError("Could not get extensions required by SDL: {}", SDL_GetError());
			exit(1);
		}
	}

	glm::ivec2 Window::GetWindowExtent() const
	{
		int width, height;
		SDL_Vulkan_GetDrawableSize(m_window, &width, &height);
		return { width, height };
	}

    void Window::SetScene(Scene* scene)
	{
	    m_scene = scene;
		if (m_scene)
		{
		    m_window_renderer->SetSceneRenderer(m_scene->GetSceneRenderer());
		}
		else
		{
		    m_window_renderer->SetSceneRenderer(nullptr);
		}
	}
}
