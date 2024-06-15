#include "EditorGuiLayer.h"

#include <Core/Engine.h>

#include "imgui.h"

namespace brr::editor
{
    EditorGuiLayer::EditorGuiLayer()
    {
    }

    void EditorGuiLayer::OnImGuiRender()
    {
        WindowImGuiLayer::OnImGuiRender();
        if (!m_window_active)
            return;

        if (!ImGui::Begin("Editor Window", &m_window_active))
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
                if (ImGui::MenuItem("Close", "Ctrl+W"))  { m_window_active = false; }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        Scene* main_scene = Engine::GetMainScene();

        bool light_active = m_light_active;
        glm::vec3 light_color = m_light_color;

        ImGui::Checkbox("Spot Light Active", &light_active);
        ImGui::ColorEdit3("Spot Light Color", &light_color.x);

        ImGui::End();

        if (light_active != m_light_active)
        {
            m_light_active = light_active;
            m_light_toggled_event.Emit (light_active);
        }

        if (light_color != m_light_color)
        {
            m_light_color = light_color;
            const glm::vec3 const_color = light_color;
            m_light_color_changed_event.Emit (const_color);
        }
    }

    void EditorGuiLayer::ToggleWindowOpen(bool is_on)
    {
        if (is_on == m_window_active)
        {
            return;
        }

        m_window_active = is_on;
    }

    Event<bool>& EditorGuiLayer::GetLightToggledEvent()
    {
        return m_light_toggled_event;
    }

    Event<glm::vec3>& EditorGuiLayer::GetLightColorChangedEvent()
    {
        return m_light_color_changed_event;
    }
}
