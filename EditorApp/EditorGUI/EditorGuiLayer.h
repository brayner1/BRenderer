#ifndef EDITOR_EDITORGUILAYER_H
#define EDITOR_EDITORGUILAYER_H
#include <Core/Events/Event.h>
#include <glm/vec3.hpp>
#include <Visualization/WindowImGuiLayer.h>

namespace brr::editor
{
    class EditorGuiLayer : public vis::WindowImGuiLayer
    {
    public:

        EditorGuiLayer();

        void OnImGuiRender() override;

        bool IsWindowOpen() const { return m_window_active; }
        void ToggleWindowOpen(bool is_on);

        Event<bool>& GetLightToggledEvent();
        Event<glm::vec3>& GetLightColorChangedEvent();

    private:

        bool m_window_active = true;

        bool m_light_active = false;
        glm::vec3 m_light_color = {.8, .8f, .8f};

        Event<bool> m_light_toggled_event;
        Event<glm::vec3> m_light_color_changed_event;
    };
}


#endif