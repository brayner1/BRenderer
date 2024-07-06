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
}
