#include "Core/App.h"
#include "Core/Window.h"
#include "Scene/Entity.h"
#include "Scene/Components/Mesh3DComponent.h"

namespace brr
{
	static const std::vector<Vertex3_PosColor> vertices{
		{glm::vec3{-0.5f, -0.5f, 0.0f}, glm::vec3{1.f, 0.f, 0.f}},
		{glm::vec3{0.5f, -0.5f, 0.0f}, glm::vec3{0.f, 1.f, 0.f}},
		{glm::vec3{.5f, .5f, 0.0f}, glm::vec3{0.f, 0.f, 1.f}},
		{glm::vec3{-.5f, .5f, 0.0f}, glm::vec3{1.f, 1.f, 1.f}}
	};

	static const std::vector<uint32_t> indices
	{
		0, 1, 2, 2, 3, 0
	};

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
		m_pWindowManager.reset(new WindowManager{ 800, 600 });


		scene_ = m_pWindowManager->GetMainWindow()->GetScene();
		Entity entity = scene_->AddEntity({});
		Mesh3DComponent& mesh = entity.AddComponent<Mesh3DComponent>();
		mesh.surfaces.emplace_back(vertices, indices);
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

			m_pWindowManager->Update();

		}
	}

	void App::Clear()
	{
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
				m_pWindowManager->ProcessWindowEvent(pEvent.window);
				if (m_pWindowManager->IsMainWindowClosed())
					m_pShouldFinish = true;
				/*Window* window;
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
				if (window->NeedToClose())
				{
					CloseWindow(pEvent.window.windowID);
				}*/
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
