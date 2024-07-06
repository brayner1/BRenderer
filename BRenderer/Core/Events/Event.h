#ifndef BRR_EVENT_H
#define BRR_EVENT_H

#include <concepts>
#include <functional>
#include <memory>

namespace brr
{
    /***************
     * EventAction *
     ***************/
    template <typename... Args>
    class EventAction
    {
    public:

        template <typename F> 
            requires std::invocable<F, Args...>
        EventAction(F&& func);

        template <typename T>
        EventAction(void(T::*func)(Args...), T* object) requires std::invocable<decltype(func), T*, Args...>;

        template <typename... Args2>
        void Call(Args2&&... args) const;

    private:
        
        std::function<void(Args...)> m_function;
    };

    /*********
     * Event *
     *********/
    template <typename... Args>
    class Event
    {
    public:

        void Subscribe(const std::shared_ptr<EventAction<Args...>>& event_action);

        void Unsubscribe(const std::shared_ptr<EventAction<Args...>>& event_action);

        template <typename... Args>
        void Emit(Args&&... args) const;

    private:

        std::vector<std::shared_ptr<EventAction<Args...>>> m_event_actions {};
    };

#include "Event.inl"
}

#endif
