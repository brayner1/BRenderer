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

		void Init();
		void MainLoop();
		void Clear();

		void OnKeyPressed(SDL_KeyCode key_code);

		vis::WindowManager* m_window_manager{};

		std::unique_ptr<Scene> m_scene;
	};
}

#endif