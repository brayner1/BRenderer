#ifndef BRR_INPUTSYSTEM_H
#define BRR_INPUTSYSTEM_H

#include <Core/Events/Event.h>

#include <SDL_keycode.h>

namespace brr
{
    class InputSystem
    {
    public:

        static void ProcessFrameInputs();

        static Event<SDL_KeyCode> input_keydown_event;
    };
}

#endif
