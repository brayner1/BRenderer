#include "EditorGuiLayer.h"

#include <imgui_internal.h>
#include <iostream>
#include <Core/Engine.h>
#include <Scene/SceneComponentsView.h>
#include <Scene/Components/LightComponents.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Scene/Components/NodeComponent.h>
#include <Scene/Components/PerspectiveCameraComponent.h>
#include <Scene/Components/Transform3DComponent.h>

#include "imgui.h"

namespace brr::editor
{
    EditorGuiLayer::EditorGuiLayer()
    {
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
    }

    void EditorGuiLayer::OnImGuiRender()
    {
        WindowImGuiLayer::OnImGuiRender();
        if (!m_window_active)
            return;

        if (m_navigator_window_size == glm::vec2(0.0))
        {
            ImVec2 size = ImGui::GetMainViewport()->Size;
            m_navigator_window_size = {size.x / 3.f, size.y};
        }
        ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Appearing, ImVec2(0.f, 0.f));
        ImGui::SetNextWindowSize ({m_navigator_window_size.x , m_navigator_window_size.y});

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
        if (!ImGui::Begin("Editor Window", &m_window_active, window_flags))
        {
            ImGui::End();
            return;
        }

        ImVec2 window_size = ImGui::GetWindowSize();
        m_navigator_window_size = {window_size.x, window_size.y};

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

        ImGuiStyle& style = ImGui::GetStyle();
        float child_w = (ImGui::GetContentRegionAvail().x - 4 * style.ItemSpacing.x) / 3;
        
        const char* light_options[] = { "Point", "Directional", "Spot", "Ambient" };
        ImGui::SetNextItemWidth(child_w);
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
        
        float child_h = (ImGui::GetContentRegionAvail().y - 4 * style.ItemSpacing.x) / 2;

        ImGui::BeginGroup();
        ImGui::Text("Scene Tree");
        ImGui::BeginChild("SceneTreeChild", ImVec2(0, child_h), ImGuiChildFlags_Border);
        DrawSceneTree(main_scene);
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left, ImGuiInputFlags_None, ImGui::GetID("SceneTreeChild")))
        {
            m_selected_entities.clear();
        }
        ImGui::EndChild();
        ImGui::EndGroup();

        Entity selected_entity = m_selected_entities.empty() ? Entity() : *m_selected_entities.begin();
        std::string entity_name = selected_entity ? selected_entity.GetComponent<NodeComponent>().GetName() : "";
        std::stringstream components_title;
        components_title << "Components";
        if (selected_entity)
        {
            components_title << " (Entity: " << entity_name << ")";
        }
        ImGui::BeginGroup();
        ImGui::Separator();
        ImGui::Text(components_title.str().c_str());
        ImGui::BeginChild("EntityComponentsChild", ImVec2(0, 0), ImGuiChildFlags_Border);
        if (selected_entity)
        {
            DrawEntityComponents(selected_entity);
        }
        ImGui::EndChild();
        ImGui::EndGroup();

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

        for (auto& root_entity : root_entities)
        {
            DrawSceneNode (&root_entity.GetComponent<NodeComponent>());
        }
    }

    void EditorGuiLayer::DrawSceneNode(NodeComponent* node_component)
    {
        static constexpr ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        ImGuiTreeNodeFlags node_flags = base_flags;
        auto children = node_component->GetChildren();
        if (children.size() == 0)
        {
            node_flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_selected_entities.contains(node_component->GetEntity()))
        {
            node_flags |= ImGuiTreeNodeFlags_Selected;
        }

        const std::string& name = node_component->GetName();
        bool node_open = ImGui::TreeNodeEx(node_component, node_flags, name.empty() ? "Object" : name.c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            if (!ImGui::GetIO().KeyCtrl)
            {
                m_selected_entities.clear();
            }
            if (!m_selected_entities.contains(node_component->GetEntity()))
            {
                m_selected_entities.emplace(node_component->GetEntity());
            }
            else
            {
                m_selected_entities.erase(node_component->GetEntity());
            }
        }

        if (node_open)
        {
            for (auto child : children)
            {
                DrawSceneNode(child);
            }
            ImGui::TreePop();
        }
    }

    void DrawTransform3DComponent(Transform3DComponent& transform_component)
    {
        glm::vec3 position = transform_component.GetLocalPosition();
        if (ImGui::InputFloat3("Position", &position.x, "%.3f", ImGuiInputTextFlags_CharsDecimal))
        {
            transform_component.SetPosition(position);
        }

        glm::vec3 euler = transform_component.GetLocalRotationEuler();
        glm::vec3 euler_degrees {glm::degrees(euler.x), glm::degrees(euler.y), glm::degrees(euler.z)};
        if (ImGui::InputFloat3("Rotation", &euler_degrees.x, "%.3f", ImGuiInputTextFlags_CharsDecimal))
        {
            euler = {glm::radians(euler_degrees.x), glm::radians(euler_degrees.y), glm::radians(euler_degrees.z)};
            transform_component.SetRotation(glm::quat(euler));
        }

        glm::vec3 scale = transform_component.GetLocalScale();
        if  (ImGui::InputFloat3("Scale", &scale.x, "%.3f", ImGuiInputTextFlags_CharsDecimal))
        {
            transform_component.SetScale(scale);
        }
    }

    void DrawPointLightComponent(PointLightComponent& point_light_component)
    {
        glm::vec3 position = point_light_component.GetPosition();
        if (ImGui::InputFloat3("Position", &position.x, "%.3f", ImGuiInputTextFlags_CharsDecimal))
        {
            point_light_component.SetPosition(position);
        }

        glm::vec3 color = point_light_component.GetColor();
        if (ImGui::ColorEdit3("Color", &color.x))
        {
            point_light_component.SetColor(color);
        }

        glm::float32 intensity = point_light_component.GetIntensity();
        if (ImGui::InputFloat("Intensity", &intensity, 0, 0, "%.3f", ImGuiInputTextFlags_CharsDecimal))
        {
            point_light_component.SetIntensity(glm::abs(intensity));
        }
        ImGui::Spacing();
    }

    void DrawSpotLightComponent(SpotLightComponent& spot_light_component)
    {
        glm::vec3 position = spot_light_component.GetPosition();
        if (ImGui::InputFloat3("Position", &position.x))
        {
            spot_light_component.SetPosition(position);
        }

        glm::vec3 direction = spot_light_component.GetDirection();
        if (ImGui::InputFloat3("Direction", &direction.x) && glm::length(direction) > glm::epsilon<float>())
        {
            spot_light_component.SetDirection(glm::normalize(direction));
        }

        glm::vec3 color = spot_light_component.GetColor();
        if (ImGui::ColorEdit3("Color", &color.x))
        {
            spot_light_component.SetColor(color);
        }

        glm::float32 intensity = spot_light_component.GetIntensity();
        if (ImGui::InputFloat("Intensity", &intensity))
        {
            spot_light_component.SetIntensity(glm::abs(intensity));
        }
        ImGui::Spacing();
    }

    void DrawDirectionalLightComponent(DirectionalLightComponent& dir_light_component)
    {
        glm::vec3 direction = dir_light_component.GetDirection();
        if (ImGui::InputFloat3("Direction", &direction.x) && glm::length(direction) > glm::epsilon<float>())
        {
            dir_light_component.SetDirection(glm::normalize(direction));
        }

        glm::vec3 color = dir_light_component.GetColor();
        if (ImGui::ColorEdit3("Color", &color.x))
        {
            dir_light_component.SetColor(color);
        }

        glm::float32 intensity = dir_light_component.GetIntensity();
        if (ImGui::InputFloat("Intensity", &intensity))
        {
            dir_light_component.SetIntensity(glm::abs(intensity));
        }
    }

    void DrawAmbientLightComponent(AmbientLightComponent& ambi_light_component)
    {
        glm::vec3 color = ambi_light_component.GetColor();
        if (ImGui::ColorEdit3("Color", &color.x))
        {
            ambi_light_component.SetColor(color);
        }

        glm::float32 intensity = ambi_light_component.GetIntensity();
        if (ImGui::InputFloat("Intensity", &intensity))
        {
            ambi_light_component.SetIntensity(glm::abs(intensity));
        }
    }

    void DrawPerspectiveCameraComponent(PerspectiveCameraComponent& camera_component)
    {
        float fov_y = glm::degrees(camera_component.GetFovY());
        if (ImGui::InputFloat("FOV Y", &fov_y))
        {
            camera_component.SetFovY(glm::radians(fov_y));
        }

        float near = camera_component.GetNear();
        if (ImGui::InputFloat("Near", &near))
        {
            camera_component.SetNear(near);
        }

        float far = camera_component.GetFar();
        if (ImGui::InputFloat("Far", &far))
        {
            camera_component.SetFar(far);
        }
    }

    void DrawMesh3DComponent(Mesh3DComponent& mesh_component)
    {
        auto& surfaces = mesh_component.GetMeshSurfaces();
        ImGui::SeparatorText("Mesh Data");
        ImGui::Text("Surfaces Number: %d", mesh_component.GetSurfaceCount());
        ImGui::SeparatorText("Mesh Surfaces");
        if (ImGui::BeginTable("SurfaceTable", 1, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Surfaces");
            ImGui::TableHeadersRow();
            uint32_t surface_idx = 0;
            for (auto& surface : surfaces)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(&surface);
                ImGui::Text("Surface %d", surface_idx);
                ImGui::Text("Vertices Count: %d", surface.GetVertices().size());
                ImGui::Text("Indices Count: %d", surface.GetIndices().size());
                ImGui::PopID();
                surface_idx++;
            }
            ImGui::EndTable();
        }
    }

    void EditorGuiLayer::DrawEntityComponents(Entity entity)
    {
        if (!entity)
            return;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

        Transform3DComponent& transform_component = entity.GetComponent<Transform3DComponent>();
        ImGui::Text ("Transform 3D Component");
        ImGui::BeginChild("Transform3DChild", {0, 85}, ImGuiChildFlags_Border);
        DrawTransform3DComponent(transform_component);
        ImGui::EndChild();

        if (entity.HasComponent<PointLightComponent>())
        {
            ImGui::Separator();
            PointLightComponent& point_light_component = entity.GetComponent<PointLightComponent>();
            ImGui::Text ("Point Light Component");
            ImGui::BeginChild("PointLightChild", {0, 85}, ImGuiChildFlags_Border);
            DrawPointLightComponent(point_light_component);
            ImGui::EndChild();
        }

        if (entity.HasComponent<SpotLightComponent>())
        {
            ImGui::Separator();
            SpotLightComponent& spot_light_component = entity.GetComponent<SpotLightComponent>();
            ImGui::Text ("Spot Light Component");
            ImGui::BeginChild("SpotLightChild", {0, 110}, ImGuiChildFlags_Border);
            DrawSpotLightComponent(spot_light_component);
            ImGui::EndChild();
        }

        if (entity.HasComponent<DirectionalLightComponent>())
        {
            ImGui::Separator();
            DirectionalLightComponent& dir_light_component = entity.GetComponent<DirectionalLightComponent>();
            ImGui::Text ("Directional Light Component");
            ImGui::BeginChild("DirLightChild", {0, 85}, ImGuiChildFlags_Border);
            DrawDirectionalLightComponent(dir_light_component);
            ImGui::EndChild();
        }

        if (entity.HasComponent<AmbientLightComponent>())
        {
            ImGui::Separator();
            AmbientLightComponent& ambi_light_component = entity.GetComponent<AmbientLightComponent>();
            ImGui::Text ("Ambient Light Component");
            ImGui::BeginChild("AmbiLightChild", {0, 60}, ImGuiChildFlags_Border);
            DrawAmbientLightComponent(ambi_light_component);
            ImGui::EndChild();
        }

        if (entity.HasComponent<PerspectiveCameraComponent>())
        {
            ImGui::Separator();
            PerspectiveCameraComponent& perspective_camera_component = entity.GetComponent<PerspectiveCameraComponent>();
            ImGui::Text ("Perspective Camera Component");
            ImGui::BeginChild("PerspCamChild", {0, 85}, ImGuiChildFlags_Border);
            DrawPerspectiveCameraComponent(perspective_camera_component);
            ImGui::EndChild();
        }

        if (entity.HasComponent<Mesh3DComponent>())
        {
            ImGui::Separator();
            Mesh3DComponent& mesh_component = entity.GetComponent<Mesh3DComponent>();
            ImGui::Text ("Mesh 3D Component");
            ImGui::BeginChild("Mesh3DChild", {0, 0}, ImGuiChildFlags_Border);
            DrawMesh3DComponent(mesh_component);
            ImGui::EndChild();
        }

        ImGui::PopStyleVar();
    }

    
}
