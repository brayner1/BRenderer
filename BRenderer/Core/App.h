#ifndef BRR_APP_H
#define BRR_APP_H
#include "Core/WindowManager.h"
#include "Renderer/Renderer.h"


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

		bool m_pShouldFinish = false;

		std::unique_ptr<WindowManager> m_pWindowManager{};

		std::unique_ptr<Scene> scene_{};
	};
}

#endif