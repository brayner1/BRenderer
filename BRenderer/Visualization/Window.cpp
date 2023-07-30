#include "Window.h"

#include <Visualization/WindowRenderer.h>
#include <Core/LogSystem.h>


namespace brr::vis
{
	Window::Window(const std::string& window_name, glm::uvec2 window_size)
	{
		const uint32_t window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
		m_pWindow = SDL_CreateWindow(window_name.c_str(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			window_size.x, window_size.y,
			window_flags);

		if (!m_pWindow)
		{
			BRR_LogError("Failed to open {} x {} window: {}", window_size.x, window_size.y, SDL_GetError());
			exit(1);
		}

		uint32_t winId = SDL_GetWindowID(m_pWindow);
		if (!winId)
		{
			BRR_LogError("Failed to get window ID: {}", SDL_GetError());
			exit(1);
		}

		m_pWindowID = winId;

		BRR_LogInfo("Window {} Created", winId);
		int width, height;
		SDL_Vulkan_GetDrawableSize(m_pWindow, &width, &height);
		const glm::ivec2 extent{ width, height };

		scene.reset(new Scene(new PerspectiveCamera(
			glm::vec3{ 2.f },
			glm::vec3{ 0.f },
			glm::radians(45.f),
			extent.x / (float)extent.y,
			0.1f, 100.f)));
	}

	void Window::CloseWindow()
	{
		if (m_pWindow)
		{
			m_pWindowRenderer.reset();
			SDL_DestroyWindow(m_pWindow);
			m_pWindow = nullptr;
			m_pWindowID = 0;
		}
	}

	Window::~Window()
	{
		CloseWindow();
		BRR_LogInfo("Window Closed");
	}

    void Window::InitWindowRenderer()
    {
		m_pWindowRenderer = std::make_unique<WindowRenderer>(this);
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
			m_pWindowRenderer->Window_Resized();
			break;
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
			m_pNeedToClose = true;
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
		m_pWindowRenderer->BeginRenderWindow();
		m_pWindowRenderer->EndRenderWindow();
    }

	void Window::GetRequiredVulkanExtensions(std::vector<const char*>& extensions) const
	{
		unsigned int extension_count;

		// Get the number of extensions required by the SDL window.
		if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow,
			&extension_count, nullptr))
		{
			BRR_LogError("Could not get the number of extensions required by SDL: {}", SDL_GetError());
			exit(1);
		}

		const size_t additional_extension_count = extensions.size();
		extensions.resize(additional_extension_count + extension_count);

		if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow,
			&extension_count, extensions.data() + additional_extension_count))
		{
			BRR_LogError("Could not get extensions required by SDL: {}", SDL_GetError());
			exit(1);
		}

		LogStreamBuffer aLogMsg = BRR_InfoStrBuff();
		aLogMsg << "Required Extensions:\n";
		for (const char* extension : extensions)
		{
			aLogMsg << "\tExtension name: " << extension << "\n";
		}
		aLogMsg.Flush();
	}

	glm::ivec2 Window::GetWindowExtent() const
	{
		int width, height;
		SDL_Vulkan_GetDrawableSize(m_pWindow, &width, &height);
		return { width, height };
	}

	vk::SurfaceKHR Window::GetVulkanSurface(vk::Instance instance)
	{
		if (!window_surface_ &&
			!SDL_Vulkan_CreateSurface(m_pWindow, instance, reinterpret_cast<VkSurfaceKHR*>(&window_surface_)))
		{
			BRR_LogError("Could not create Vulkan surface of SDL window {}: {}", m_pWindowID, SDL_GetError());
		}

		return window_surface_;
	}
}
