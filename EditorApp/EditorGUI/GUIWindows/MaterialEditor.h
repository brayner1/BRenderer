#ifndef BRR_EDITOR_MATERIALEDITOR_H
#define BRR_EDITOR_MATERIALEDITOR_H

#include <Core/Ref.h>
#include <Visualization/Resources/Material.h>

namespace brr::editor {

    class MaterialEditor {
    public:
        MaterialEditor();
        void OnImGuiRender();
    private:
        //Ref<vis::Material> m_Material;
        int m_selected_material_idx = 0;
    };

} // namespace brr::editor

#endif // BRR_EDITOR_MATERIALEDITOR_H