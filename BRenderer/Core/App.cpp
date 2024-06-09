#include <random>
#include <Core/App.h>
#include <Core/LogSystem.h>
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Scene/Components/PerspectiveCameraComponent.h>

#include "Importer/Importer.h"
#include "Renderer/RenderThread.h"
#include "Scene/Components/LightComponents.h"
#include "Scene/Components/Transform3DComponent.h"

static bool isLightOn = false;
static brr::Entity light_entity;

namespace brr
{
	App::App() : m_scene(nullptr)
	{}

	void App::Run()
	{
		LogSystem::SetPattern("[%Y-%m-%d %T.%e] [%^%l%$] [%!] [%s:%#]\n%v\n");
		LogSystem::SetLevel(LogLevel::Debug);

		Init();
		MainLoop();
		Clear();
	}

	void App::Init()
	{
		m_keydown_event.Subscribe(std::make_shared<EventAction<SDL_KeyCode>>(& App::OnKeyPressed, this));
		
		m_window_manager = std::make_unique<vis::WindowManager>(800, 600);

		m_scene.reset(new Scene());
		m_scene->InitSceneRenderer();

		Entity light_entity = m_scene->Add3DEntity({});
		light_entity.AddComponent<PointLightComponent>(glm::vec3(0.0, 6.0, 0.0), glm::vec3(1.0, 0.8, 0.8), 2.0);

		SceneImporter::LoadFileIntoScene("Resources/Monkey/Monkey.obj", m_scene.get());

		Entity camera_entity = m_scene->Add3DEntity(Entity());
		PerspectiveCameraComponent& camera_component = camera_entity.AddComponent<PerspectiveCameraComponent>(glm::radians(45.0), 0.1f, 100.f);
		camera_component.LookAt({8.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {0.f, -1.f, 0.f});

		glm::vec3 proj_point = camera_component.TransformToViewportCoords({-4.681385, 1.117334, 3.768916}, 800.f / 600.f);
		BRR_LogInfo("Point (0, 0, 0) on Viewport Space: {}", glm::to_string(proj_point));

		m_window_manager->GetMainWindow()->GetSceneView().SetCamera(&camera_component);
	}

	void App::MainLoop()
	{
		while(!m_should_finish)
		{
			SDL_Event sdl_event;
			while (SDL_PollEvent(&sdl_event))
			{
				ProcessEvent(sdl_event);
			}

			m_scene->Update();

            render::RenderThread::MainThread_SyncUpdate();
		}
	}

	void App::Clear()
	{
		m_scene.reset();
		m_window_manager.reset();
	}

    void App::OnKeyPressed(SDL_KeyCode key_code)
    {
		if (key_code == SDL_KeyCode::SDLK_l)
		{
			if (!isLightOn)
			{
				isLightOn = true;
				light_entity = m_scene->Add3DEntity({});
				//light_entity.AddComponent<PointLightComponent>(glm::vec3(0.0, 6.0, -3.0), glm::vec3(0.7, 0.7, 1.0), 3.0);
				light_entity.AddComponent<SpotLightComponent>(glm::vec3(0.0, 6.0, 0.0), glm::vec3(0.0, -1.0, 0.0),
					                                          glm::radians(45.0/2.0), glm::vec3(1.0), 3.0);
				//light_entity.AddComponent<DirectionalLightComponent>(glm::vec3(0.0, -0.42, 0.91), glm::vec3(1.0), 1.0);
				//light_entity.AddComponent<AmbientLightComponent>(glm::vec3(0.2, 0.2, 0.2), 1);
			}
			else
			{
				isLightOn = false;
				m_scene->RemoveEntity(light_entity);
			}
		}
		else if (key_code == SDL_KeyCode::SDLK_u)
		{
			if (isLightOn)
			{
				static std::default_random_engine random_engine;
				std::uniform_real_distribution<float> distrib (0.0, 1.0);
				SpotLightComponent& spot_light = light_entity.GetComponent<SpotLightComponent>();
				spot_light.SetColor({distrib(random_engine), distrib(random_engine), distrib(random_engine)});
			}
		}
    }

    void App::ProcessEvent(SDL_Event& pEvent)
	{
		switch ((SDL_EventType)pEvent.type)
		{
			// Last window closed, close application
			case SDL_QUIT:
			{
				m_should_finish = true;
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
				m_window_manager->ProcessWindowEvent(pEvent.window);
				if (m_window_manager->IsMainWindowClosed())
				{
				    m_should_finish = true;
					BRR_LogDebug("Main Window closed. Stopping application.");
				}
				break;
			}
			case SDL_SYSWMEVENT: break; // This event is disabled by default. Encouraged to avoid if you can find less platform-specific way to accomplish your goals.
		    case SDL_KEYDOWN:
		    {
                EventEmitter<SDL_KeyCode>::Emit(m_keydown_event, SDL_KeyCode(pEvent.key.keysym.sym));
		        break;
		    }
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
