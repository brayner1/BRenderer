#include "WindowManager.h"

#include <Core/LogSystem.h>
#include <Renderer/Vulkan/VulkanRenderDevice.h>

#include "Renderer/RenderThread.h"

namespace brr::vis
{
	static std::unique_ptr<WindowManager> s_window_manager {};

	WindowManager::WindowManager(uint32_t width, uint32_t height)
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			BRR_LogInfo("Could not initialize SDL: {}", SDL_GetError());
			exit(1);
		}
		BRR_LogInfo("SDL Initialized.");

		m_main_window = std::make_unique<Window>("Vulkan Test", glm::uvec2{width, height});
		m_main_window_ID = m_main_window->GetWindowID();
		m_main_window_closed = false;

		render::RenderThread::InitializeRenderingThread(render::RenderAPI::Vulkan, m_main_window->GetSDLWindowHandle());

		m_render_device = render::VKRD::GetSingleton();

		m_main_window->InitWindowRenderer();

		BRR_LogInfo("WindowManager initialized.");
	}

    WindowManager* WindowManager::Instance()
    {
		return s_window_manager.get();
    }

    void WindowManager::InitWindowManager(uint32_t width,
                                           uint32_t height)
    {
		if (s_window_manager)
		{
		    BRR_LogError("Error: Can't call 'WindowManager::InitWindowManager' when WindowManager instance is already initialized.");
			return;
		}
		s_window_manager.reset(new WindowManager(width, height));
    }

    void WindowManager::DestroyWindowManager()
    {
		if (!s_window_manager)
		{
		    BRR_LogError("Error: Can't call 'WindowManager::DestroyWindowManager' when WindowManager instance is not initialized.");
			return;
		}
		s_window_manager.reset();
    }

    WindowManager::~WindowManager()
	{
		m_secondaryWindowsID_index_map.clear();
		m_secondary_windows.clear();

		m_main_window.reset();

        render::RenderThread::StopRenderingThread();

		SDL_Quit();
	}

	void WindowManager::CloseWindow(WindowID pWindowID)
	{
		// If closing main window, close the application
		if (pWindowID == m_main_window_ID)
		{
			CloseMainWindow();
			return;
		}

		auto it = m_secondaryWindowsID_index_map.find(pWindowID);
		if (it == m_secondaryWindowsID_index_map.end())
			return;

		const WindowID window_index = it->second;
		m_secondary_windows[window_index]->CloseWindow();

		m_secondaryWindowsID_index_map.erase(pWindowID);
		m_secondary_windows.erase(m_secondary_windows.begin() + window_index);
	}

    void WindowManager::CloseMainWindow()
    {
		m_main_window->CloseWindow();
		m_main_window_closed = true;

		m_secondary_windows.clear();
		m_secondaryWindowsID_index_map.clear();
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
