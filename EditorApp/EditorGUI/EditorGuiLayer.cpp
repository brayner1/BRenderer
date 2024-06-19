#include "EditorGuiLayer.h"

#include <Core/Engine.h>
#include <Scene/SceneComponentsView.h>
#include <Scene/Components/LightComponents.h>
#include <Scene/Components/Transform3DComponent.h>

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
        
        const char* light_options[] = { "Point", "Directional", "Spot", "Ambient" };
        ImGui::Combo("Light Type", &m_current_light_option, light_options, std::size(light_options));
        ImGui::SameLine();
        if (ImGui::Button("Create Light"))
        {
            Entity light_entity = main_scene->Add3DEntity({});
            switch (m_current_light_option)
            {
            case 0:
                BRR_LogInfo("EditorUI: Creating Point Light.");
                light_entity.AddComponent<PointLightComponent>(glm::vec3(), glm::vec3(1.0, 1.0, 1.0), 1.0);
                break;
            case 1:
                light_entity.AddComponent<DirectionalLightComponent>(glm::vec3(0.0, -1.0, 0.0), glm::vec3(1.0, 1.0, 1.0), 1.0);
                BRR_LogInfo("EditorUI: Creating Directional Light.");
                break;
            case 2:
                light_entity.AddComponent<SpotLightComponent>(glm::vec3(), glm::vec3(0.0, -1.0, 0.0), glm::radians(45.0f), glm::vec3(1.0, 1.0, 1.0), 1.0);
                BRR_LogInfo("EditorUI: Creating Spot Light.");
                break;
            case 3:
                light_entity.AddComponent<AmbientLightComponent>(glm::vec3(1.0, 1.0, 1.0), 0.3);
                BRR_LogInfo("EditorUI: Creating Ambient Light.");
                break;
            }
        }

        ImGuiStyle& style = ImGui::GetStyle();
        float child_w = (ImGui::GetContentRegionAvail().x - 4 * style.ItemSpacing.x) / 2;

        {
            SceneComponentsView<PointLightComponent> point_light_view (main_scene);
            ImGui::BeginGroup();
            ImGui::Text("Point Lights");
            ImGui::BeginChild("PointLightChild", ImVec2(child_w, 260), ImGuiChildFlags_Border);
            uint32_t light_idx = 0;
            for (auto [point_light ]: point_light_view)
            {
                Entity light_entity = point_light.GetEntity();
                {
                    std::stringstream light_stream {};
                    light_stream << "Point Light " << light_idx << " (Entity: " << light_entity << ")";
                    ImGui::SeparatorText(light_stream.str().c_str());
                }

                {
                    std::stringstream light_stream {};
                    light_stream << "Point Light " << light_idx;
                    ImGui::PushID(light_stream.str().c_str());
                }

                glm::vec3 position = point_light.GetPosition();
                if (ImGui::InputFloat3("Position", &position.x))
                {
                    point_light.SetPosition(position);
                }

                glm::vec3 color = point_light.GetColor();
                if (ImGui::ColorEdit3("Color", &color.x))
                {
                    point_light.SetColor(color);
                }

                glm::float32 intensity = point_light.GetIntensity();
                if (ImGui::InputFloat("Intensity", &intensity))
                {
                    point_light.SetIntensity(glm::abs(intensity));
                }
                ImGui::Spacing();
                ImGui::PopID();
                ++light_idx;
            }
            ImGui::EndChild();
            ImGui::EndGroup();
        }
        ImGui::SameLine();
        {
            SceneComponentsView<DirectionalLightComponent> dir_light_view (main_scene);
            ImGui::BeginGroup();
            ImGui::Text("Directional Lights");
            ImGui::BeginChild("DirectionalLightChild", ImVec2(child_w, 260), ImGuiChildFlags_Border);
            uint32_t light_idx = 0;
            for (auto [dir_light]: dir_light_view)
            {
                Entity light_entity = dir_light.GetEntity();
                {
                    std::stringstream light_stream {};
                    light_stream << "Directional Light " << light_idx << " (Entity: " << light_entity << ")";
                    ImGui::SeparatorText(light_stream.str().c_str());
                }

                {
                    std::stringstream light_stream {};
                    light_stream << "Directional Light " << light_idx;
                    ImGui::PushID(light_stream.str().c_str());
                }

                glm::vec3 direction = dir_light.GetDirection();
                if (ImGui::InputFloat3("Direction", &direction.x) && glm::length(direction) > glm::epsilon<float>())
                {
                    dir_light.SetDirection(glm::normalize(direction));
                }

                glm::vec3 color = dir_light.GetColor();
                if (ImGui::ColorEdit3("Color", &color.x))
                {
                    dir_light.SetColor(color);
                }

                glm::float32 intensity = dir_light.GetIntensity();
                if (ImGui::InputFloat("Intensity", &intensity))
                {
                    dir_light.SetIntensity(glm::abs(intensity));
                }
                ImGui::Spacing();
                ImGui::PopID();
                ++light_idx;
            }
            ImGui::EndChild();
            ImGui::EndGroup();
        }

        {
            SceneComponentsView<SpotLightComponent> spot_light_view (main_scene);
            ImGui::BeginGroup();
            ImGui::Text("Spot Lights");
            ImGui::BeginChild("SpotLightChild", ImVec2(child_w, 260), ImGuiChildFlags_Border);
            uint32_t light_idx = 0;
            for (auto [spot_light ]: spot_light_view)
            {
                Entity light_entity = spot_light.GetEntity();
                {
                    std::stringstream light_stream {};
                    light_stream << "Spot Light " << light_idx << " (Entity: " << light_entity << ")";
                    ImGui::SeparatorText(light_stream.str().c_str());
                }

                {
                    std::stringstream light_stream {};
                    light_stream << "Spot Light " << light_idx;
                    ImGui::PushID(light_stream.str().c_str());
                }

                glm::vec3 position = spot_light.GetPosition();
                if (ImGui::InputFloat3("Position", &position.x))
                {
                    spot_light.SetPosition(position);
                }

                glm::vec3 direction = spot_light.GetDirection();
                if (ImGui::InputFloat3("Direction", &direction.x) && glm::length(direction) > glm::epsilon<float>())
                {
                    spot_light.SetDirection(glm::normalize(direction));
                }

                glm::vec3 color = spot_light.GetColor();
                if (ImGui::ColorEdit3("Color", &color.x))
                {
                    spot_light.SetColor(color);
                }

                glm::float32 intensity = spot_light.GetIntensity();
                if (ImGui::InputFloat("Intensity", &intensity))
                {
                    spot_light.SetIntensity(glm::abs(intensity));
                }
                ImGui::Spacing();
                ImGui::PopID();
                ++light_idx;
            }
            ImGui::EndChild();
            ImGui::EndGroup();
        }
        ImGui::SameLine();
        {
            SceneComponentsView<AmbientLightComponent> amb_light_view (main_scene);
            ImGui::BeginGroup();
            ImGui::Text("Ambient Lights");
            ImGui::BeginChild("AmbientLightChild", ImVec2(child_w, 260), ImGuiChildFlags_Border);
            uint32_t light_idx = 0;
            for (auto [amb_light ]: amb_light_view)
            {
                Entity light_entity = amb_light.GetEntity();
                {
                    std::stringstream light_stream {};
                    light_stream << "Ambient Light " << light_idx << " (Entity: " << light_entity << ")";
                    ImGui::SeparatorText(light_stream.str().c_str());
                }

                {
                    std::stringstream light_stream {};
                    light_stream << "Ambient Light " << light_idx;
                    ImGui::PushID(light_stream.str().c_str());
                }

                glm::vec3 color = amb_light.GetColor();
                if (ImGui::ColorEdit3("Color", &color.x))
                {
                    amb_light.SetColor(color);
                }

                glm::float32 intensity = amb_light.GetIntensity();
                if (ImGui::InputFloat("Intensity", &intensity))
                {
                    amb_light.SetIntensity(glm::abs(intensity));
                }
                ImGui::Spacing();
                ImGui::PopID();
                ++light_idx;
            }
            ImGui::EndChild();
            ImGui::EndGroup();
        }

        

        ImGui::End();
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

    void EditorGuiLayer::DrawSceneTree(Scene* scene)
    {
        auto root_entities = scene->GetRootEntities();
    }
}
