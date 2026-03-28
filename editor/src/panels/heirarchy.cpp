#include "panels.h"
#include <imgui.h>

// I don't know if I should connect this with the rendergraph ds

// TODO:
// Fix the camera component so that when hovering over the panels the scene does not get updated. Also in the camera
// make a toggle that works when I press c etc.
// Editor camera and other cameras (Editor camera is the priority now.`)
// New entity add more mesh does not work gives excpetion.
namespace ic::panels
{

void heirarchy_draw(ic::RenderScene *scene, ic::Entity &selected)
{
        ImGui::Begin("Hierarchy");

        if (!scene)
        {
                ImGui::End();
                return;
        }

        // ---- Toolbar: create / delete ----
        if (ImGui::Button("+ Entity"))
        {
                scene->CreateEntityWithName("New Entity");
        }

        if (selected && selected.IsValid())
        {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.20f, 0.20f, 1.0f));
                if (ImGui::Button("Delete"))
                {
                        scene->DestroyEntity(
                            selected);  // make sure this removes all the components of that entity as well.
                        selected = {};
                }
                ImGui::PopStyleColor(2);
        }

        ImGui::Separator();

        // ---- Entity list ----
        std::vector<ic::Entity> entities = scene->GetAllEntities();

        for (auto &e : entities)
        {
                if (!e.IsValid())
                        continue;

                std::string label = e.GetName();
                if (label.empty())
                        label = "(unnamed)";

                bool isSelected = (selected && selected == e);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth |
                                           ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                           (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

                ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)e, flags, "%s", label.c_str());

                if (ImGui::IsItemClicked())
                {
                        selected = e;
                }

                // Right-click context menu per entity
                if (ImGui::BeginPopupContextItem())
                {
                        if (ImGui::MenuItem("Rename"))
                        {
                                // TODO: inline rename with InputText
                        }
                        if (ImGui::MenuItem("Duplicate"))
                        {
                                ic::Entity dup = scene->CreateEntityWithName(e.GetName() + "_copy");
                                if (e.HasComponent<ic::TransformComponent>())
                                {
                                        auto &src = e.GetComponent<ic::TransformComponent>();
                                        auto &dst = dup.GetComponent<ic::TransformComponent>();
                                        dst.SetPosition(src.position);
                                        dst.SetRotation(src.rotation);
                                        dst.SetScale(src.scale);
                                }
                                if (e.HasComponent<ic::MeshComponent>())
                                {
                                        auto &src = e.GetComponent<ic::MeshComponent>();
                                        dup.AddComponent<ic::MeshComponent>().SetMesh(src.modelID);
                                }
                                selected = dup;
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Delete"))
                        {
                                scene->DestroyEntity(e);
                                if (selected && selected == e)
                                        selected = {};
                        }
                        ImGui::EndPopup();
                }
        }

        // Right-click on empty space to create entity
        if (ImGui::BeginPopupContextWindow("##HierarchyBg",
                                           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
                if (ImGui::MenuItem("Create Empty Entity"))
                        scene->CreateEntityWithName("Entity");
                ImGui::EndPopup();
        }

        ImGui::End();
}

}  // namespace ic::panels
