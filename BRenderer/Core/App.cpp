#include "App.h"
#include <Core/LogSystem.h>
#include <Core/Inputs/InputSystem.h>

#include <Importer/Importer.h>

#include <Renderer/RenderThread.h>

#include <Scene/Entity.h>
#include <Scene/Scene.h>
#include <Scene/Components/LightComponents.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Scene/Components/PerspectiveCameraComponent.h>

#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"

#include <random>

#include "Engine.h"

static bool isLightOn = false;
static brr::Entity light_entity;

namespace brr
{
	App::App()
	{}

    App::~App()
    {}

    void App::Run()
	{
		Init();
		MainLoop();
		Clear();
	}

	void App::Init()
	{
	    LogSystem::SetLevel(LogLevel::Debug);

	    Engine::InitEngine();
		m_window_manager = Engine::GetWindowManager();

		OnInit();
		//InputSystem::Instance()->GetKeydownEvent().Subscribe(std::make_shared<EventAction<SDL_KeyCode>>(&App::OnKeyPressed, this));

		//Scene* main_scene = Engine::GetMainScene();

		//main_scene->InitSceneRenderer();

		//Entity light_entity = main_scene->Add3DEntity({});
		//light_entity.AddComponent<PointLightComponent>(glm::vec3(0.0, 6.0, 0.0), glm::vec3(1.0, 0.8, 0.8), 2.0);

		//SceneImporter::LoadFileIntoScene("Resources/Monkey/Monkey.obj", main_scene);

		//Entity camera_entity = main_scene->Add3DEntity(Entity());
		//PerspectiveCameraComponent& camera_component = camera_entity.AddComponent<PerspectiveCameraComponent>(glm::radians(45.0), 0.1f, 100.f);
		//camera_component.LookAt({8.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {0.f, -1.f, 0.f});

		//glm::vec3 proj_point = camera_component.TransformToViewportCoords({-4.681385, 1.117334, 3.768916}, 800.f / 600.f);
		//BRR_LogInfo("Point (0, 0, 0) on Viewport Space: {}", glm::to_string(proj_point));

		//m_window_manager->GetMainWindow()->GetSceneView().SetCamera(&camera_component);
	}

	void App::MainLoop()
	{
		while(!m_window_manager->IsMainWindowClosed())
		{
			Engine::UpdateMainLoop();
		}
	}

	void App::Clear()
	{
		OnShutdown();
		Engine::ShutdownEngine();
	}

  //  void App::OnKeyPressed(SDL_KeyCode key_code)
  //  {
		//Scene* main_scene = Engine::GetMainScene();
		//if (key_code == SDL_KeyCode::SDLK_l)
		//{
		//	if (!isLightOn)
		//	{
		//		isLightOn = true;
		//		light_entity = main_scene->Add3DEntity({});
		//		//light_entity.AddComponent<PointLightComponent>(glm::vec3(0.0, 6.0, -3.0), glm::vec3(0.7, 0.7, 1.0), 3.0);
		//		light_entity.AddComponent<SpotLightComponent>(glm::vec3(0.0, 6.0, 0.0), glm::vec3(0.0, -1.0, 0.0),
		//			                                          glm::radians(45.0/2.0), glm::vec3(1.0), 3.0);
		//		//light_entity.AddComponent<DirectionalLightComponent>(glm::vec3(0.0, -0.42, 0.91), glm::vec3(1.0), 1.0);
		//		//light_entity.AddComponent<AmbientLightComponent>(glm::vec3(0.2, 0.2, 0.2), 1);
		//	}
		//	else
		//	{
		//		isLightOn = false;
		//		main_scene->RemoveEntity(light_entity);
		//	}
		//}
		//else if (key_code == SDL_KeyCode::SDLK_u)
		//{
		//	if (isLightOn)
		//	{
		//		static std::default_random_engine random_engine;
		//		std::uniform_real_distribution<float> distrib (0.0, 1.0);
		//		SpotLightComponent& spot_light = light_entity.GetComponent<SpotLightComponent>();
		//		spot_light.SetColor({distrib(random_engine), distrib(random_engine), distrib(random_engine)});
		//	}
		//}
  //  }
}
