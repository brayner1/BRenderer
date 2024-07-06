#ifndef BRR_SCENECOMPONENTSVIEW_H
#define BRR_SCENECOMPONENTSVIEW_H
//#include <Scene/Entity.h>
#include <Scene/Scene.h>

namespace brr
{
    template <typename... Args>
    class SceneComponentsView
    {
        using view_type = entt::basic_view<entt::entity, entt::get_t<Args...>, entt::exclude_t<>>;
    public:

        struct ComponentViewIterator
        {
            ComponentViewIterator operator++() noexcept
            {
                ++m_iter;
                return *this;
            }

            ComponentViewIterator operator++(int) noexcept
            {
                ComponentViewIterator orig = *this;
                ++m_iter;
                return orig;
            }

            std::tuple<Args&...> operator*()
            {
                return std::tuple<Args&...>(m_view->template get<Args>(*m_iter)...);
            }
            //Entity operator*() { return Entity(*m_iter, m_scene); }

            template <typename T>
            T& GetComponent()
            {
                return m_view->template get<T>(*m_iter);
            }

            bool operator==(const ComponentViewIterator& other) const
            {
                return m_iter == other.m_iter;
            }

            bool operator!=(const ComponentViewIterator& other) const
            {
                return !(*this == other);
            }

        private:

            ComponentViewIterator(view_type::iterator view_iter, const view_type& view)
            : m_iter(view_iter),
              m_view(&view)
            {}

            //template <typename>
            friend class SceneComponentsView;
            view_type::iterator m_iter;
            const view_type* m_view;
        };

        SceneComponentsView(Scene* scene)
        : m_scene(scene),
          m_components_view(m_scene->m_registry.view<Args...>())
        {}

        ComponentViewIterator begin() { return { m_components_view.begin(), m_components_view }; }
        ComponentViewIterator end() { return { m_components_view.end(), m_components_view }; }

    private:
        Scene* m_scene;
        view_type m_components_view;
    };

}

#endif