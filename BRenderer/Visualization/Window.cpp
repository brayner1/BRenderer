#include "Window.h"

#include <Core/LogSystem.h>

#include <Visualization/SceneView.h>
#include <Renderer/RenderThread.h>

namespace brr::vis
{
	Window::Window(const std::string& window_name, glm::uvec2 window_size)
	: m_scene_view(this)
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
	}

	void Window::CloseWindow()
	{
		if (m_window)
		{
            render::RenderThread::WindowRenderCmd_DestroyWindowRenderer(m_window);
			SDL_DestroyWindow(m_window);
			m_window = nullptr;
			m_window_id = 0;
		}
	}

	Window::~Window()
	{
		m_scene_view.UnsetCamera();
		CloseWindow();
		BRR_LogInfo("Window Closed");
	}

    void Window::InitWindowRenderer()
    {
		render::RenderThread::WindowRenderCmd_InitializeWindowRenderer(m_window);
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
			render::RenderThread::WindowRenderCmd_Resize(m_window);
			//m_window_renderer->Window_Resized();
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED: break;
		case SDL_WINDOWEVENT_MINIMIZED:
			BRR_LogInfo("Window {} minimized.", m_window_id);
			render::RenderThread::WindowRenderCmd_Resize(m_window);
			m_minimized = true;
			break;
		case SDL_WINDOWEVENT_MAXIMIZED: break;
		case SDL_WINDOWEVENT_RESTORED:
			BRR_LogInfo("Window {} restored.", m_window_id);
			render::RenderThread::WindowRenderCmd_Resize(m_window);
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

    void Window::RenderImGuiLayer()
    {
		if (m_imgui_layer)
		{
		    m_imgui_layer->OnImGuiRender();
		}
    }

    void Window::SetImGuiLayer(std::shared_ptr<WindowImGuiLayer> imgui_layer)
    {
		m_imgui_layer = std::move(imgui_layer);
    }

    glm::ivec2 Window::GetWindowSize() const
	{
		int width, height;
		SDL_GetWindowSize(m_window, &width, &height);
		return { width, height };
	}
}
