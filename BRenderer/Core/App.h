#ifndef BRR_APP_H
#define BRR_APP_H
#include "Core/Window.h"
#include "Renderer/Renderer.h"

namespace brr
{

	class App
	{
	public:
		App();

		void Run();

		void CloseWindow(uint32_t pWindowID);

	private:
		void ProcessEvent(SDL_Event& pEvent);

		void Init();
		void MainLoop();
		void Clear();

		bool m_pShouldFinish = false;

		uint32_t m_pMainWindowID = 0;
		std::unique_ptr<Window> m_pMainWindow {};

		// Here for posterior support for multiple windows
		std::unordered_map<uint32_t, uint32_t> m_pSecondaryWindows_Id_Index_Map {};
		std::vector<std::unique_ptr<Window>> m_pSecondaryWindows {};

		render::Renderer* m_pRenderer;
	};

}

#endif