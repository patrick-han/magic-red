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


    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Engine", nullptr, nullptr);

    }

    void initVulkan() {
        vk::ApplicationInfo appInfo("Hello Triangle", VK_MAKE_API_VERSION(1, 0, 0, 0), "Magic Red", VK_MAKE_API_VERSION(1, 0, 0, 0), VK_API_VERSION_1_2);

        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> glfwExtensionsVector(glfwExtensions, glfwExtensions + glfwExtensionCount);

        #ifdef NDEBUG
            std::cout << "Run release" << std::endl;
        #else
            std::cout << "Run debug" << std::endl;
            glfwExtensionsVector.push_back("VK_EXT_debug_utils");
        #endif




        uint32_t extensionCount = 0;
        vk::Result a = vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::cout << extensionCount << " extensions supported\n";
        std::cout << ROOT_DIR << std::endl;

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