#include "InputSystem.h"

#include <Core/LogSystem.h>
#include <Core/Events/Event.h>

#include <SDL2/SDL.h>
#include <backends/imgui_impl_sdl2.h>

#include "Visualization/WindowManager.h"

namespace brr
{
    namespace
    {
        void ProcessEvent(SDL_Event& pEvent)
	    {
			vis::WindowManager* window_manager = vis::WindowManager::Instance();
		    switch ((SDL_EventType)pEvent.type)
		    {
			    // Last window closed, close application
			    case SDL_QUIT:
			    {
				    if (window_manager)
				    {
				        window_manager->CloseMainWindow();
				    }
				    break;
			    }
			    case SDL_APP_TERMINATING: break;
			    case SDL_APP_LOWMEMORY: break;
			    case SDL_APP_WILLENTERBACKGROUND: break;
			    case SDL_APP_DIDENTERBACKGROUND: break;
			    case SDL_APP_WILLENTERFOREGROUND: break;
			    case SDL_APP_DIDENTERFOREGROUND: break;
			    case SDL_LOCALECHANGED: break;
			    case SDL_DISPLAYEVENT: break;
			    case SDL_WINDOWEVENT:
			    {
					
					if (window_manager)
					{
						window_manager->ProcessWindowEvent(pEvent.window);
						if (window_manager->IsMainWindowClosed())
						{
						    BRR_LogDebug("Main Window closed. Stopping application.");
						}
					}
				    break;
			    }
			    case SDL_SYSWMEVENT: break; // This event is disabled by default. Encouraged to avoid if you can find less platform-specific way to accomplish your goals.
		        case SDL_KEYDOWN:
		        {
                    EventEmitter<SDL_KeyCode>::Emit(InputSystem::input_keydown_event, SDL_KeyCode(pEvent.key.keysym.sym));
		            break;
		        }
			    case SDL_KEYUP: break;
			    case SDL_TEXTEDITING: break;
			    case SDL_TEXTINPUT: break;
			    case SDL_KEYMAPCHANGED: break;
			    case SDL_MOUSEMOTION: break;
			    case SDL_MOUSEBUTTONDOWN: break;
			    case SDL_MOUSEBUTTONUP: break;
			    case SDL_MOUSEWHEEL: break;
			    case SDL_JOYAXISMOTION: break;
			    case SDL_JOYBALLMOTION: break;
			    case SDL_JOYHATMOTION: break;
			    case SDL_JOYBUTTONDOWN: break;
			    case SDL_JOYBUTTONUP: break;
			    case SDL_JOYDEVICEADDED: break;
			    case SDL_JOYDEVICEREMOVED: break;
			    case SDL_CONTROLLERAXISMOTION: break;
			    case SDL_CONTROLLERBUTTONDOWN: break;
			    case SDL_CONTROLLERBUTTONUP: break;
			    case SDL_CONTROLLERDEVICEADDED: break;
			    case SDL_CONTROLLERDEVICEREMOVED: break;
			    case SDL_CONTROLLERDEVICEREMAPPED: break;
			    case SDL_CONTROLLERTOUCHPADDOWN: break;
			    case SDL_CONTROLLERTOUCHPADMOTION: break;
			    case SDL_CONTROLLERTOUCHPADUP: break;
			    case SDL_CONTROLLERSENSORUPDATE: break;
			    case SDL_FINGERDOWN: break;
			    case SDL_FINGERUP: break;
			    case SDL_FINGERMOTION: break;
			    case SDL_DOLLARGESTURE: break;
			    case SDL_DOLLARRECORD: break;
			    case SDL_MULTIGESTURE: break;
			    case SDL_CLIPBOARDUPDATE: break;
			    case SDL_DROPFILE: break;
			    case SDL_DROPTEXT: break;
			    case SDL_DROPBEGIN: break;
			    case SDL_DROPCOMPLETE: break;
			    case SDL_AUDIODEVICEADDED: break;
			    case SDL_AUDIODEVICEREMOVED: break;
			    case SDL_SENSORUPDATE: break;
			    case SDL_RENDER_TARGETS_RESET: break;
			    case SDL_RENDER_DEVICE_RESET: break;
			    case SDL_POLLSENTINEL: break;
			    case SDL_USEREVENT: break;
			    case SDL_LASTEVENT: break;
			    default:;
		    }
	    }
    }

	Event<SDL_KeyCode> InputSystem::input_keydown_event {};

    void InputSystem::ProcessFrameInputs()
    {
        SDL_Event sdl_event;
		while (SDL_PollEvent(&sdl_event))
		{
			//send SDL event to imgui for handling
            ImGui_ImplSDL2_ProcessEvent(&sdl_event);

			ProcessEvent(sdl_event);
		}
    }
}
