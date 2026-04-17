#include "panels.h"
#include <imgui.h>

// I don't know if I should connect this with the rendergraph ds

// TODO:
// Fix the camera component so that when hovering over the panels the scene does not get updated. Also in the camera
// make a toggle that works when I press c etc.
// Editor camera and other cameras (Editor camera is the priority now.`)
// New entity add more mesh does not work gives excpetion.
static uint8_t count_lights(ic::RenderScene *scene)
{
        uint8_t count = 0;
        scene->Each<ic::LightComponent>([&](auto) { count++; });
        return count;
}

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
                        /** FIX: Delete entity does not kill the components from the screen */
                        scene->DestroyEntity(selected);
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
                uint8_t n = count_lights(scene);
                if (ImGui::BeginMenu("Create Lighting"))
                {
                        if (ImGui::MenuItem("Point Light"))
                        {
                                std::string name   = "PointLight_" + std::to_string(n);
                                auto        entity = scene->CreateEntityWithName(name);

                                // Add components
                                auto &lc     = entity.AddComponent<ic::LightComponent>();
                                lc.type      = ic::LightType::Point;
                                lc.color     = glm::vec3(1.0f, 1.0f, 1.0f);
                                lc.intensity = 1.0f;
                                lc.range     = 10.0f;
                        }

                        if (ImGui::MenuItem("Directional Light"))
                        {
                                std::string name   = "DirectionalLight_" + std::to_string(n);
                                auto        entity = scene->CreateEntityWithName(name);

                                auto &lc     = entity.AddComponent<ic::LightComponent>();
                                lc.type      = ic::LightType::Directional;
                                lc.color     = glm::vec3(1.0f, 0.95f, 0.85f);  // warm sunlight default
                                lc.intensity = 1.0f;
                        }

                        if (ImGui::MenuItem("Spot Light"))
                        {
                                std::string name   = "SpotLight_" + std::to_string(n);
                                auto        entity = scene->CreateEntityWithName(name);

                                auto &lc      = entity.AddComponent<ic::LightComponent>();
                                lc.type       = ic::LightType::Spot;
                                lc.color      = glm::vec3(1.0f);
                                lc.intensity  = 1.0f;
                                lc.range      = 20.0f;
                                lc.innerAngle = 12.5f;
                                lc.outerAngle = 25.0f;
                        }

                        ImGui::EndMenu();
                }
                ImGui::EndPopup();
        }
        ImGui::End();
}

}  // namespace ic::panels
