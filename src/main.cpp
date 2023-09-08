#include <GLFW/glfw3.h>
#include "vulkan/vulkan.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include "RootDir.h"

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

class Engine {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    // Vulkan
    vk::ApplicationInfo appInfo;
    std::vector<const char*> glfwExtensionsVector;
    std::vector<const char*> layers;
    vk::UniqueInstance instance;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Engine", nullptr, nullptr);
        glfwSetKeyCallback(window, key_callback);
    }

    void createInstance() {
        // Specify application and engine info
        appInfo = vk::ApplicationInfo("Hello Triangle", VK_MAKE_API_VERSION(1, 0, 0, 0), "Magic Red", VK_MAKE_API_VERSION(1, 0, 0, 0), VK_API_VERSION_1_2);

        // Get extensions required by GLFW for VK surface rendering. Should contain atleast VK_KHR_surface
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        glfwExtensionsVector = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // MoltenVK requires
        vk::InstanceCreateFlagBits instanceCreateFlagBits;
        #ifdef __APPLE__
            std::cout << "Running on an Apple device, adding appropriate extension and instance creation flag bits" << std::endl;
            glfwExtensionsVector.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            instanceCreateFlagBits = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
        #endif

        // Enable validation layers and creation of debug messenger if building debug
        if (enableValidationLayers) {
            std::cout << "Debug build" << std::endl;
            glfwExtensionsVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layers.push_back("VK_LAYER_KHRONOS_validation");
        } else {
            std::cout << "Release build" << std::endl;
        }

        // Create instance
        auto instanceCreateInfo = vk::InstanceCreateInfo(
            instanceCreateFlagBits,
            &appInfo,
            static_cast<uint32_t>(layers.size()),
            layers.data(),
            static_cast<uint32_t>(glfwExtensionsVector.size()),
            glfwExtensionsVector.data()
        );
        instance = vk::createInstanceUnique(instanceCreateInfo);
    }

    void createDebugMessenger() {
        if (!enableValidationLayers) {
            return;
        }
        auto dldi = vk::DispatchLoaderDynamic(*instance, vkGetInstanceProcAddr);
        auto debugUtilsMessenger = instance->createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT{ {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            debugCallback },
        nullptr, dldi);
    }

    void initVulkan() {
        createInstance();
        createDebugMessenger();


        // uint32_t extensionCount = 0;
        // vk::Result a = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        // std::cout << extensionCount << " extensions supported\n";
        // std::cout << ROOT_DIR << std::endl;

    }

    void mainLoop() {
        


        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        glfwDestroyWindow(window);

        glfwTerminate();
    }
};

int main() {
    Engine engine;

    try {
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}