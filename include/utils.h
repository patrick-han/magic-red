#pragma once
#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

// Logging

#define MRLOG(msg) \
    std::cout << "[MRLOG] " << msg << std::endl

#define DEBUG(msg) \
    std::cout << "[DEBUG] " << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl

#define MRCERR(msg) \
    std::cerr << "[CERR] " << msg << std::endl


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


// Callbacks

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}