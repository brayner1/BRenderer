#include "MaterialEditor.h"

#include <imgui.h>
#include <Core/Engine.h>
#include <Scene/SceneComponentsView.h>
#include <Scene/Components/Mesh3DComponent.h>

using namespace brr::editor;

MaterialEditor::MaterialEditor()
{
}

void MaterialEditor::OnImGuiRender()
{
    Scene* main_scene = Engine::GetMainScene();

    if (!main_scene)
    {
        ImGui::Text("No scene loaded.");
        return;
    }

    std::vector<Ref<vis::Material>> material_vector;

    SceneComponentsView<Mesh3DComponent> mesh_component_view (main_scene);
    for (auto iter : mesh_component_view)
    {
        Mesh3DComponent& mesh_component = std::get<0>(iter);
        std::vector<render::SurfaceID> surface_ids = mesh_component.GetMesh()->GetSurfacesIDs();
        for (auto& surface_id : surface_ids)
        {
            Ref<vis::Material> surface_material = mesh_component.GetMesh()->GetSurfaceMaterial(surface_id);
            auto material_iter = std::ranges::find(material_vector, surface_material);
            if (material_iter == material_vector.end())
            {
                material_vector.push_back(surface_material);
            }
        }
    }

    ImGui::Begin("Material Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

     static ImGuiComboFlags flags = 0;
    // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
    std::string preview_string = "Material" + std::to_string(m_selected_material_idx);
    const char* combo_preview_value = preview_string.c_str();
    if (ImGui::BeginCombo("Scene Materials", combo_preview_value, flags))
    {
        for (int n = 0; n < material_vector.size(); n++)
        {
            std::string combo_item_string = "Material" + std::to_string(n);
            const bool is_selected = (m_selected_material_idx == n);
            if (ImGui::Selectable(combo_item_string.c_str(), is_selected))
                m_selected_material_idx = n;

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    Ref<vis::Material> material = m_selected_material_idx < material_vector.size() ? material_vector[m_selected_material_idx] : Ref<vis::Material>();

    if (!material)
    {
        ImGui::Text("No material selected.");
        ImGui::End();
        return;
    }

    glm::vec3 diffuse_color = material->GetDiffuseColor();
    if (ImGui::ColorEdit3("Diffuse Color", &diffuse_color.x))
    {
        material->SetDiffuseColor(diffuse_color);
    }

    glm::vec3 emissive_color = material->GetEmissiveColor();
    if (ImGui::ColorEdit3("Emissive Color", &emissive_color.x))
    {
        material->SetEmissiveColor(emissive_color);
    }

    float metallic = material->GetMetallic();
    if (ImGui::DragFloat("Metallic", &metallic, 0.01f, 0.0f, 1.0f))
    {
        material->SetMetallic(metallic);
    }

    float roughness = material->GetRoughness();
    if (ImGui::DragFloat("Roughness", &roughness, 0.01f, 0.0f, 1.0f))
    {
        material->SetRoughness(roughness);
    }

    ImGui::End();
}
