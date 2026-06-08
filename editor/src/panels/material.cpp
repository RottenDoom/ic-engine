#include "editor.h"
#include <imgui.h>
#include <core/assets/asset_manager.h>

namespace ic
{

void EditorSystem::DrawMaterialEditor(Entity &e)
{
        ImGui::Begin("Material");
        auto &comp = e.GetComponent<MeshComponent>();

        Model *model = (Model *)AssetManager::Get().GetAsset(comp.modelID);

        if (!model)
        {
                ImGui::End();
                return;
        }

        Mesh *mesh = model->GetMesh(m_State.selectedMesh);

        if (!mesh)
        {
                ImGui::End();
                return;
        }

        if (m_State.selectedSubmesh >= mesh->primitives.size())
        {
                ImGui::End();
                return;
        }

        MeshPrimitive &prim = mesh->primitives[m_State.selectedSubmesh];

        Material *mat = model->GetMaterial(prim.materialIndex);

        if (!mat)
        {
                ImGui::End();
                return;
        }

        // Change the properties from here however this might not get updated yet
        ImGui::SliderFloat("Roughness", &mat->pbr.metallicFactor, 0.0f, 1.0f);

        ImGui::End();
}

}  // namespace ic
