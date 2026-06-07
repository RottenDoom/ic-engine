#include "editor.h"
#include <imgui.h>
#include <core/assets/asset_manager.h>

namespace ic
{

void EditorSystem::DrawMaterialEditor(Entity &e)
{
        ImGui::Begin("Material");

        if (!e.IsValid())
        {
                ImGui::TextDisabled("No Entity selected");
                ImGui::End();
                return;
        }

        // TODO: change this part of the project
        MeshComponent comp  = e.GetComponent<MeshComponent>();
        Model        *model = (Model *)AssetManager::Get().GetAsset(comp.modelID);

        Mesh          *mesh = model->GetMesh(m_State.selectedMesh);
        MeshPrimitive &prim = mesh->primitives[m_State.selectedSubmesh];

        Material *mat = model->GetMaterial(prim.materialIndex);

        // Most basic property change
        ImGui::SliderFloat("Roughness", &mat->pbr.metallicFactor, 0.0f, 1.0f);

        ImGui::End();
}

}  // namespace ic
