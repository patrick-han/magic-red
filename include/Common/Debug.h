#pragma once
#include <vulkan/vulkan.hpp>
#include "Common/Log.h"

// Debug and platform build defines

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

#ifdef __APPLE__
    const bool appleBuild = true;
#else
    const bool appleBuild = false;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    MRCERR("validation layer: " << pCallbackData->pMessage);
    // std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
