#include "Core/Window.h"
#include "Renderer/Renderer.h"

namespace brr
{
	uint32_t Window::InitWindow()
	{
		constexpr uint32_t SCREEN_WIDTH = 600;
		constexpr uint32_t SCREEN_HEIGHT = 600;

		const uint32_t window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
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
		int width, height;
		SDL_Vulkan_GetDrawableSize(m_pWindow, &width, &height);
		const glm::ivec2 extent{ width, height };

		scene.reset(new Scene(new PerspectiveCamera(
			glm::vec3{ 2.f },
			glm::vec3{ 0.f },
			glm::radians(45.f),
			extent.x / (float)extent.y,
			0.1f, 100.f)));

		return m_pWindowID;
	}

	void Window::CloseWindow()
	{
		if (m_pWindow)
		{
			SDL_DestroyWindow(m_pWindow);
			m_pWindow = nullptr;
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
		case SDL_WINDOWEVENT_RESIZED:
			render::Renderer::GetRenderer()->Window_Resized(this);
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

	void Window::GetRequiredVulkanExtensions(std::vector<const char*>& extensions) const
	{
		unsigned int extension_count;

		// Get the number of extensions required by the SDL window.
		if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow,
			&extension_count, nullptr))
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not get the number of extensions required by SDL: %s", SDL_GetError());
			exit(1);
		}

		const size_t additional_extension_count = extensions.size();
		extensions.resize(additional_extension_count + extension_count);

		if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow,
			&extension_count, extensions.data() + additional_extension_count))
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not get extensions required by SDL: %s", SDL_GetError());
			exit(1);
		}

		SDL_Log("Required Extensions");
		for (const char* extension : extensions)
		{
			SDL_Log("\tExtension name: %s", extension);
		}
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
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create Vulkan surface of SDL window %u: %s", m_pWindowID, SDL_GetError());
		}

		return window_surface_;
	}
}