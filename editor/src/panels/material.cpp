#include "editor.h"
#include <imgui.h>
#include <core/assets/asset_manager.h>
#include <core/assets/types/material.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ic
{

bool DrawTextureSlot(const char *label, TextureHandle &handle, AssetManager &assets)
{
        bool changed = false;

        ImGui::PushID(label);

        ImGui::Text("%s", label);

        Texture *tex = assets.GetAsset<Texture>(handle);

        // get gl handle here or something
        if (tex)
        {
                ImGui::Image((ImTextureID)(uintptr_t)tex->GetGPUHandle(), ImVec2(64, 64));
        }
        else
        {
                ImGui::Button("Empty", ImVec2(64, 64));
        }

        ImGui::SameLine();

        if (tex)
                ImGui::Text("%s", tex->GetName());
        else
                ImGui::TextDisabled("No Texture");

        if (ImGui::Button("Select"))
        {
                IC_CORE_INFO("Open Texture picker");
                // OpenTexturePicker(&handle);
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear"))
        {
                handle  = INVALID_ID;
                changed = true;
        }

        ImGui::PopID();

        return changed;
}

void EditorSystem::DrawMaterialEditor(Entity &e)
{
        ImGui::Begin("Material");
        auto &comp = e.GetComponent<MeshComponent>();

        Model *model = AssetManager::Get().GetAsset<Model>(comp.modelID);

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

        // TODO: give option to edit only this material or every primitive sharing this handle
        MaterialAsset *matAsset = AssetManager::Get().GetAsset<MaterialAsset>(prim.materialHandle);

        if (!matAsset)
        {
                ImGui::End();
                return;
        }

        bool changed = EditMaterial(&matAsset->GetMaterial());

        if (changed)
                matAsset->MarkDirty();

        // if e.hascomponent<materialcomponent> e.getcomponent<materialcomponent>().editmaterial(mat) -> sends this to
        // graphics api and adds it to the gpu reloads the texture and shader and changes the model and removes any
        // texture that is not needed later. Lifetime of texture needs to be studied tho.

        ImGui::End();
}

bool EditorSystem::EditMaterial(Material *mat)
{
        bool changed = false;
        if (ImGui::CollapsingHeader("Surface", ImGuiTreeNodeFlags_DefaultOpen))
        {
                changed |= ImGui::ColorEdit4("Base Color", glm::value_ptr(mat->pbr.baseColorFactor));
                changed |= DrawTextureSlot("Base Color Texture", mat->pbr.baseColorTexture, AssetManager::Get());

                changed |= ImGui::SliderFloat("Metallic", &mat->pbr.metallicFactor, 0.0f, 1.0f);
                changed |= ImGui::SliderFloat("Roughness", &mat->pbr.roughnessFactor, 0.0f, 1.0f);
                changed |= DrawTextureSlot("Metallic Roughness Texture",
                                           mat->pbr.metallicRoughnessTexture,
                                           AssetManager::Get());

                changed |= DrawTextureSlot("Normal Texture", mat->normalTexture.ref, AssetManager::Get());
                changed |= ImGui::SliderFloat("Normal Scale", &mat->normalTexture.scale, 0.0f, 1.0f);

                changed |= DrawTextureSlot("Occlusion Texture", mat->occlusionTexture.ref, AssetManager::Get());
                changed |= ImGui::SliderFloat("Occlusion Strength", &mat->occlusionTexture.strength, 0.0f, 1.0f);

                changed |= DrawTextureSlot("Emissive Texture",
                                           mat->emissiveTexture.emissiveTexture,
                                           AssetManager::Get());
                changed |= ImGui::ColorEdit3("Emissive Color", glm::value_ptr(mat->emissiveTexture.emissiveFactor));
                changed |= ImGui::SliderFloat("Emissive Strength", &mat->emissiveTexture.emissiveStrength, 0.0f, 10.0f);
        }
        return changed;
}

}  // namespace ic
