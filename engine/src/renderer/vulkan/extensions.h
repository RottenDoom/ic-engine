#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include "defines.h"

#include <vulkan/vulkan.h>

namespace ic::vk::extensions
{
bool checkInstanceExtensionSupport(const std::vector<const char *> &requiredExtensions);
bool checkValidationLayerSupport(const std::vector<const char *> &requiredLayers);

std::vector<const char *> getRequiredInstanceExtensions(bool enableValidation);
}  // namespace ic::vk::extensions

#endif
