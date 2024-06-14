#ifndef BRR_APP_H
#define BRR_APP_H

#define REGISTER_DEFAULT_APP(TYPE) struct AppRegister_##TYPE \
	{ \
	    AppRegister_##TYPE() \
		{} \
	}; \
	static AppRegister_##TYPE s_##TYPE_register = AppRegister_##TYPE{}; \

#include <Visualization/WindowManager.h>

#include "Events/Event.h"

namespace brr
{
	class App
	{
	public:
		App();

		virtual ~App();

		// Should be called in main to initialize and start the application's main loop
		void Run();

		// Called after Engine is initialized
		virtual void OnInit() {}
		// Called before Engine is shutdown
		virtual void OnShutdown() {}

	private:

		void Init();
		void MainLoop();
		void Clear();

		//void OnKeyPressed(SDL_KeyCode key_code);

		vis::WindowManager* m_window_manager {};
	};

	REGISTER_DEFAULT_APP(App)
}

#endif