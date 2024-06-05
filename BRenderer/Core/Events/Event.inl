/***************
* EventAction *
***************/

template <typename ... Args>
template <typename F> requires std::invocable<F, Args...>
EventAction<Args...>::EventAction(F&& func)
: m_function(std::forward<F>(func))
{}

template <typename ... Args>
template <typename T>
EventAction<Args...>::EventAction(void(T::* func)(Args...), T* object)
requires std::invocable<decltype(func), T*, Args...>
{
    m_function = [object, func](Args... args)
    {
        std::invoke(func, object, std::forward<Args>(args)...);
    };
}

/*********
 * Event *
 *********/

template <typename ... Args>
void EventAction<Args...>::Call(Args&&... args) const
{
    m_function(std::forward<Args>(args)...);
}

template <typename ... Args>
void Event<Args...>::Subscribe(const std::shared_ptr<EventAction<Args...>>& event_action)
{
    auto event_iter = std::find(m_event_actions.begin(), m_event_actions.end(), event_action);
    if (event_iter != m_event_actions.end())
    {
        return;
    }

    m_event_actions.push_back(event_action);
}

template <typename ... Args>
void Event<Args...>::Unsubscribe(const std::shared_ptr<EventAction<Args...>>& event_action)
{
    auto event_iter = std::find(m_event_actions.begin(), m_event_actions.end(), event_action);
    if (event_iter == m_event_actions.end())
    {
        return;
    }

    m_event_actions.erase(event_iter);
}

template <typename ... Args>
void Event<Args...>::Emit(Args&&... args) const
{
    for (auto& event_action : m_event_actions)
    {
        event_action->Call(std::forward<Args>(args)...);
    }
}

/****************
 * EventEmitter *
 ****************/

template <typename ... Args>
void EventEmitter<Args...>::Emit(Args&&... args) const
{
    m_event.Emit(std::forward<Args>(args)...);
}

template <typename ... Args>
void EventEmitter<Args...>::Emit(const Event<Args...>& event,
    Args&&... args)
{
    EventEmitter<Args...> event_emitter (event);
    event_emitter.Emit(std::forward<Args>(args)...);
}