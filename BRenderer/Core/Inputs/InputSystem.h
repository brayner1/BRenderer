#ifndef BRR_INPUTSYSTEM_H
#define BRR_INPUTSYSTEM_H

#include <Core/Events/Event.h>

#include <SDL_keycode.h>

namespace brr
{
    class InputSystem
    {
    public:

        static InputSystem* Instance();

        Event<SDL_KeyCode>& GetKeydownEvent() { return m_input_keydown_event; }

    private:
        friend class Engine;

        InputSystem();

        void ProcessFrameInputs();

    private:

        Event<SDL_KeyCode> m_input_keydown_event;
    };
}

#endif
