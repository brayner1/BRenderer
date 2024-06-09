#ifndef BRR_EVENT_H
#define BRR_EVENT_H

#include <concepts>
#include <functional>
#include <memory>

namespace brr
{
    template <typename... Args>
    class EventEmitter;

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

        void Call(Args&&... args) const;

    private:
        
        std::function<void(Args...)> m_function;
    };

    template <>
    class EventAction<>
    {
    public:

        template <typename F> 
            requires std::invocable<F>
        EventAction(F&& func);

        template <typename T>
        EventAction(void (T::*func)(),
                    T* object) requires std::invocable<decltype(func), T*>;

        void Call() const;

    private:
        
        std::function<void()> m_function;
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

    private:
        friend class brr::EventEmitter<Args...>;

        void Emit(Args&&... args) const;

        std::vector<std::shared_ptr<EventAction<Args...>>> m_event_actions {};
    };

    /****************
     * EventEmitter *
     ****************/
    template <typename... Args>
    class EventEmitter
    {
    public:

        EventEmitter(const Event<Args...>& event)
        : m_event(event)
        {}

        void Emit(Args&&... args) const;

        static void Emit(const Event<Args...>& event, Args&&... args);

    private:

        const Event<Args...>& m_event;
    };

#include "Event.inl"
}

#endif
