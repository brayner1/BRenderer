#ifndef EDITOR_EDITORGUILAYER_H
#define EDITOR_EDITORGUILAYER_H

#include <unordered_set>
#include <Core/Events/Event.h>
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Visualization/WindowImGuiLayer.h>

#include "GUIWindows/MaterialEditor.h"

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

        void DrawSceneTree(Scene* scene);
        void DrawSceneNode(NodeComponent* node_component);

        void DrawEntityComponents(Entity entity);

        bool m_window_active = true;

        int m_current_light_option = 0;

        bool m_light_active = false;
        glm::vec3 m_light_color = {.8, .8f, .8f};

        glm::vec2 m_navigator_window_size {};

        std::unordered_set<Entity> m_selected_entities {};

        Event<bool> m_light_toggled_event;
        Event<glm::vec3> m_light_color_changed_event;

        std::unique_ptr<MaterialEditor> m_material_editor;
    };
}


#endif