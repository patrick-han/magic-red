#pragma once
#include <vulkan/vulkan.h>
#include <Common/Log.h>
#include <Common/Compiler/Unused.h>
#include <nvrhi/vulkan.h>

// Debug and platform build defines

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    MRVAL("validation layer: " << pCallbackData->pMessage);
    UNUSED(messageSeverity);
    UNUSED(messageType);
    UNUSED(pUserData);
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


// Nvidia RHI
class MessageCallbackImpl final : public nvrhi::IMessageCallback
{
public:
    void message( nvrhi::MessageSeverity severity, const char* messageText ) override
    {
        const char* severityString = [&severity]()
        {
            using nvrhi::MessageSeverity;
            switch ( severity )
            {
            case MessageSeverity::Info: return "[INFO]";
            case MessageSeverity::Warning: return "[WARNING]";
            case MessageSeverity::Error: return "[ERROR]";
            case MessageSeverity::Fatal: return "[### FATAL ERROR ###]";
            default: return "[unknown]";
            }
        }();

        // std::cout << "NVRHI::" << severityString << " " << messageText << std::endl << std::endl;
        MRLOG("NVRHI::" << severityString << " " << messageText);

        if ( severity == nvrhi::MessageSeverity::Fatal )
        {
            // std::cout << "Fatal error encountered, look above ^" << std::endl;
            // std::cout << "=====================================" << std::endl;
            MRLOG("Fatal error encountered, look above ^");
            MRLOG("=====================================");
        }
    }
};