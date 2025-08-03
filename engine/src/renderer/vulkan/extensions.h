#pragma once
#include "defines.h"

namespace ic::vk::extensions
{
    bool checkInstanceExtensionSupport(const std::vector<const char*>& requiredExtensions);
    bool checkValidationLayerSupport(const std::vector<const char*>& requiredLayers);

    std::vector<const char*> getRequiredInstanceExtensions(bool enableValidation);
} // namespace ic::vk::extensions
