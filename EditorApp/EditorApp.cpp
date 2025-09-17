#include <random>
#include <Core/thirdpartiesInc.h>
#include <Core/App.h>
#include <Core/Inputs/InputSystem.h>
#include <Core/Engine.h>
#include <Scene/Components/LightComponents.h>

#include "EditorGUI/EditorGuiLayer.h"
#include "Importer/Importer.h"
#include "Scene/Components/PerspectiveCameraComponent.h"
#include "Scene/Components/Transform3DComponent.h"

using namespace brr;

class EditorApp : public brr::App
{
public:
	EditorApp()
	: App()
	{
	    m_light_color_changed_action = std::make_shared<EventAction<glm::vec3>>(&EditorApp::OnLightColorChanged, this);
	    m_light_toggled_action = std::make_shared<EventAction<bool>>(&EditorApp::OnLightToggled, this);
	}

	void OnInit() override
	{
		m_window_manager = Engine::GetWindowManager();
		m_editor_gui_layer = std::make_shared<editor::EditorGuiLayer>();
		m_window_manager->GetMainWindow()->SetImGuiLayer(m_editor_gui_layer);
		m_editor_gui_layer->GetLightColorChangedEvent().Subscribe(m_light_color_changed_action);
		m_editor_gui_layer->GetLightToggledEvent().Subscribe(m_light_toggled_action);
		InputSystem::Instance()->GetKeydownEvent().Subscribe(std::make_shared<EventAction<SDL_KeyCode>>(&EditorApp::OnKeyPressed, this));

		Scene* main_scene = Engine::GetMainScene();

		main_scene->InitSceneRenderer();

		Entity light_entity = main_scene->Add3DEntity("Point Light");
		light_entity.GetComponent<Transform3DComponent>().SetPosition(glm::vec3(0.0, 6.0, 0.0));
		light_entity.AddComponent<PointLightComponent>(glm::vec3(1.0, 0.8, 0.8), 2.0);

		SceneImporter::LoadFileIntoScene("Resources/Monkey/Monkey.obj", main_scene);

		Entity camera_entity = main_scene->Add3DEntity("Camera");
		PerspectiveCameraComponent& camera_component = camera_entity.AddComponent<PerspectiveCameraComponent>(glm::radians(45.0), 0.1f, 100.f);
		camera_component.LookAt({8.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {0.f, -1.f, 0.f});

		glm::vec3 proj_point = camera_component.TransformToViewportCoords({-4.681385, 1.117334, 3.768916}, 800.f / 600.f);
		BRR_LogInfo("Point (0, 0, 0) on Viewport Space: {}", glm::to_string(proj_point));

		m_window_manager->GetMainWindow()->GetSceneView().SetCamera(&camera_component);
	}

private:

	void OnKeyPressed(SDL_KeyCode key_code)
	{

		if (key_code == SDL_KeyCode::SDLK_u)
		{
			if (m_light_is_on)
			{
				static std::default_random_engine random_engine;
				std::uniform_real_distribution<float> distrib (0.0, 1.0);
				SpotLightComponent& spot_light = light_entity.GetComponent<SpotLightComponent>();
				spot_light.SetColor({distrib(random_engine), distrib(random_engine), distrib(random_engine)});
			}
		}
		else if (key_code == SDLK_m)
		{
		    m_editor_gui_layer->ToggleWindowOpen(!m_editor_gui_layer->IsWindowOpen());
		}
	}

	void OnLightToggled(bool value)
	{
		Scene* main_scene = Engine::GetMainScene();
		m_light_is_on = value;
	    if (m_light_is_on)
		{
			light_entity = main_scene->Add3DEntity("Spot Light");
			//light_entity.AddComponent<PointLightComponent>(glm::vec3(0.0, 6.0, -3.0), glm::vec3(0.7, 0.7, 1.0), 3.0);
			light_entity.AddComponent<SpotLightComponent>(m_light_color, 3.0,
					                                      glm::radians(45.0/2.0));
			//light_entity.AddComponent<DirectionalLightComponent>(glm::vec3(0.0, -0.42, 0.91), glm::vec3(1.0), 1.0);
			//light_entity.AddComponent<AmbientLightComponent>(glm::vec3(0.2, 0.2, 0.2), 1);
		}
		else
		{
			m_light_is_on = false;
			main_scene->RemoveEntity(light_entity);
		}
	}

	void OnLightColorChanged(glm::vec3 color)
	{
		m_light_color = color;
	    if (m_light_is_on)
		{
			SpotLightComponent& spot_light = light_entity.GetComponent<SpotLightComponent>();
			spot_light.SetColor(m_light_color);
		}
	}

	bool m_light_is_on = false;
	glm::vec3 m_light_color = {.8, .8f, .8f};
    Entity light_entity;

	vis::WindowManager* m_window_manager {};

	std::shared_ptr<editor::EditorGuiLayer> m_editor_gui_layer {};

	std::shared_ptr<EventAction<glm::vec3>> m_light_color_changed_action;
	std::shared_ptr<EventAction<bool>> m_light_toggled_action;
};

int main(int argc, char* argv[])
{
	EditorApp application;
	application.Run();
	return 0;
}