#ifndef BRR_APP_H
#define BRR_APP_H
#include <Visualization/WindowManager.h>

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

		std::unique_ptr<vis::WindowManager> m_pWindowManager{};

		Scene* scene_;
	};
}

#endif