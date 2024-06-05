#ifndef BRR_APP_H
#define BRR_APP_H
#include <Visualization/WindowManager.h>

#include "Events/Event.h"

namespace brr
{
	class App
	{
	public:
		App();

		void Run();

	private:
		void ProcessEvent(SDL_Event& pEvent);

		void Init();
		void MainLoop();
		void Clear();

		void OnKeyPressed(SDL_KeyCode key_code);

		Event<SDL_KeyCode> m_keydown_event;

		bool m_should_finish = false;

		std::unique_ptr<vis::WindowManager> m_window_manager{};

		std::unique_ptr<Scene> m_scene;
	};
}

#endif