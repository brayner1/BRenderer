#include "Engine.h"

#include <Core/Inputs/InputSystem.h>

#include <Importer/Importer.h>

#include <Renderer/RenderThread.h>

#include <Visualization/WindowManager.h>

#include <Scene/Components/LightComponents.h>

#include <memory>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>


namespace brr
{
    static bool s_is_initialized = false;
    static std::unique_ptr<vis::WindowManager> s_window_manager {};
    static std::unique_ptr<InputSystem> s_input_system {};

    static std::unique_ptr<Scene> s_main_scene {};

    void Engine::InitEngine()
    {
        LogSystem::SetPattern("[%Y-%m-%d %T.%e] [%^%l%$] [%!] [%s:%#]\n%v\n");
        s_window_manager.reset(new vis::WindowManager(800, 600));
        s_input_system.reset(new InputSystem());
        s_main_scene.reset(new Scene());

        s_is_initialized = true;
    }

    void Engine::ShutdownEngine()
    {
        s_main_scene.reset();
		s_input_system.reset();
		s_window_manager.reset();
    }

    bool Engine::IsInitialized()
    {
        return s_is_initialized;
    }

    vis::WindowManager* Engine::GetWindowManager()
    {
        return s_window_manager.get();
}

    InputSystem* Engine::GetInputSystem()
    {
        return s_input_system.get();
    }

    Scene* Engine::GetMainScene()
    {
        return s_main_scene.get();
    }

    void Engine::UpdateMainLoop()
    {
        s_input_system->ProcessFrameInputs();

        if (s_window_manager->IsMainWindowClosed())
				return;

        s_main_scene->Update();

        // imgui new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		//some imgui UI to test
		ImGui::ShowDemoWindow();

		//make imgui calculate internal draw structures
		ImGui::Render();

        render::RenderThread::MainThread_SyncUpdate();
    }
}
