#include "panels.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace ic::panels
{

void light_panel_draw(LightComponent &lc)
{
        ImGui::SeparatorText("Light");

        // Type selector
        const char *typeLabels[] = {"Directional", "Point", "Spot"};
        int         typeIndex    = static_cast<int>(lc.type);
        if (ImGui::Combo("Type", &typeIndex, typeLabels, 3))
                lc.type = static_cast<LightType>(typeIndex);

        ImGui::Checkbox("Enabled", &lc.enabled);

        // Color + intensity on one row
        ImGui::ColorEdit3("Color", glm::value_ptr(lc.color));
        ImGui::DragFloat("Intensity", &lc.intensity, 0.05f, 0.0f, 100.0f);

#ifndef NDEBUG

        // TODO: these models do not show a material.
        if (lc.type == LightType::Point && !lc.visible)
        {
                lc.modelID = AssetManager::Get().GetRegistry()->GetAssetId("point_model");
                lc.visible = true;
        }

        if (lc.type == LightType::Spot && !lc.visible)
        {
                lc.modelID = AssetManager::Get().GetRegistry()->GetAssetId("spot_model");
                lc.visible = true;
        }
#endif

        // Range (point and spot only)
        if (lc.type == LightType::Point || lc.type == LightType::Spot)
        {
                ImGui::DragFloat("Range", &lc.range, 0.1f, 0.0f, 500.0f);
        }

        // Cone angles (spot only)
        if (lc.type == LightType::Spot)
        {
                ImGui::DragFloat("Inner Angle", &lc.innerAngle, 0.5f, 0.0f, lc.outerAngle - 0.5f);
                ImGui::DragFloat("Outer Angle", &lc.outerAngle, 0.5f, lc.innerAngle + 0.5f, 90.0f);
        }

        // Placeholder toggle
        ImGui::BeginDisabled();
        ImGui::Checkbox("Cast Shadows (soon)", &lc.castShadows);
        ImGui::EndDisabled();
}

}  // namespace ic::panels