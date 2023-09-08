#include "utils.h" // Includes vulkan and glfw headers!

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <shaderc/shaderc.hpp>

#include <iostream>
#include <vector>
#include <set>
#include <stdexcept>
#include <cstdlib>
#include "RootDir.h"


const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;


shaderc::SpvCompilationResult compileShader(std::string shaderSource, shaderc_shader_kind shaderKind, const char *inputFileName) {
    // TODO: Not sure what the cost of doing this compiler init everytime is, fine for now
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc::SpvCompilationResult shaderModuleCompile = compiler.CompileGlslToSpv(shaderSource, shaderKind, inputFileName, options);
    if (shaderModuleCompile.GetCompilationStatus() != shaderc_compilation_status_success) {
        MRCERR(shaderModuleCompile.GetErrorMessage());
        exit(0); // TODO: Do something else instead?
    }
    return shaderModuleCompile;
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

    // Instance
    vk::ApplicationInfo appInfo;
    std::vector<const char*> glfwExtensionsVector;
    std::vector<const char*> layers;
    vk::UniqueInstance instance;

    // Surface and devices
    vk::UniqueSurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice device;

    // Queues
    size_t graphicsQueueFamilyIndex;
    size_t presentQueueFamilyIndex;
    std::set<uint32_t> uniqueQueueFamilyIndices;
    std::vector<uint32_t> FamilyIndices;

    // Swapchain
    vk::UniqueSwapchainKHR swapChain;
    vk::Format swapChainFormat;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::UniqueImageView> swapChainImageViews;

    // Shaders
    vk::UniqueShaderModule vertexShaderModule;
    vk::UniqueShaderModule fragmentShaderModule;

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
        if (appleBuild) {
            std::cout << "Running on an Apple device, adding appropriate extension and instance creation flag bits" << std::endl;
            glfwExtensionsVector.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            instanceCreateFlagBits = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
        }

        // Enable validation layers and creation of debug messenger if building debug
        if (enableValidationLayers) {
            MRLOG("Debug build");
            glfwExtensionsVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layers.push_back("VK_LAYER_KHRONOS_validation");
        } else {
            MRLOG("Release build");
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

    void createSurface() {
        VkSurfaceKHR surfaceTmp;
        VkResult err = glfwCreateWindowSurface(*instance, window, nullptr, &surfaceTmp);
        if (err != VK_SUCCESS) {
            MRLOG("Surface creation unsuccessful!");
            exit(0);
        }
        surface = vk::UniqueSurfaceKHR(surfaceTmp, *instance);
    }

    void initPhysicalDevice() {
        std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();
        for (vk::PhysicalDevice& d : physicalDevices) {
            MRLOG("Physical device enumerated: " << d.getProperties().deviceName);
        }

        std::vector<vk::PhysicalDevice>::iterator findIfIterator;
        if (appleBuild) {
            findIfIterator = std::find_if(physicalDevices.begin(), physicalDevices.end(), 
                [](const vk::PhysicalDevice& physicalDevice) -> char* {
                    return strstr(physicalDevice.getProperties().deviceName, "Apple"); // Returns null pointer if not found
                }
            );
            physicalDevice = physicalDevices[std::distance(physicalDevices.begin(), findIfIterator)];
            vk::PhysicalDeviceProperties physDeviceProperties = physicalDevice.getProperties();
            MRLOG("Chosen deviceName: " << physDeviceProperties.deviceName);
        } else {
            physicalDevice = physicalDevices[0]; // TODO: By Default, just select the first physical device
        }
    }

    void findQueueFamilyIndices() {
        // Find graphics and present queue family indices
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(),
            std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                [](vk::QueueFamilyProperties const& qfp) {
                    return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
                }
            )
        );
        presentQueueFamilyIndex = 0u;
        for (unsigned long queueFamilyIndex = 0ul; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++) {
            // Check if a given queue family on our device supports presentation to the surface that was created
            if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(queueFamilyIndex), surface.get())) {
                presentQueueFamilyIndex = queueFamilyIndex;
            }
        }
        MRLOG("Graphics and Present queue family indices, respectively: " << graphicsQueueFamilyIndex << ", " << presentQueueFamilyIndex);

        uniqueQueueFamilyIndices = {
            static_cast<uint32_t>(graphicsQueueFamilyIndex),
            static_cast<uint32_t>(presentQueueFamilyIndex)
        };

        FamilyIndices = {
            uniqueQueueFamilyIndices.begin(),
            uniqueQueueFamilyIndices.end()
        };

    }

    void createDevice() {
        // Creation of logical device requires queue creation info as well as extensions + layers we want

        // For each queue family, create a single queue
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 0.0f;
        uint32_t queueCountPerFamily = 1;
        for (auto& queueFamilyIndex : uniqueQueueFamilyIndices) {
            queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(),
                static_cast<uint32_t>(queueFamilyIndex), queueCountPerFamily, &queuePriority });
        }

        // Device extensions
        std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        if (appleBuild) {
            deviceExtensions.push_back("VK_KHR_portability_subset");
        }
        

        vk::DeviceCreateInfo deviceCreateInfo = {
            vk::DeviceCreateFlags(),
            static_cast<uint32_t>(queueCreateInfos.size()),
            queueCreateInfos.data(),
            0u,
            nullptr,
            static_cast<uint32_t>(deviceExtensions.size()),
            deviceExtensions.data()
        };
        device = physicalDevice.createDeviceUnique(deviceCreateInfo);

    }

    void createSwapchain() {
        // If the graphics and presentation queue family indices are different, allow concurrent access to object from multiple queue families
        struct SM {
            vk::SharingMode sharingMode;
            uint32_t familyIndicesCount;
            uint32_t* familyIndicesDataPtr;
        } sharingModeUtil  = { (graphicsQueueFamilyIndex != presentQueueFamilyIndex) ?
                           SM{ vk::SharingMode::eConcurrent, 2u, FamilyIndices.data() } :
                           SM{ vk::SharingMode::eExclusive, 0u, static_cast<uint32_t*>(nullptr) } };
        
         // Needed for validation warnings
        vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
        std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(*surface);

        swapChainFormat = vk::Format::eB8G8R8A8Unorm;
        vk::Extent2D extent  = { WINDOW_WIDTH, WINDOW_HEIGHT };
        uint32_t imageCount = 2;

        vk::SwapchainCreateInfoKHR swapChainCreateInfo = {
            {}, 
            surface.get(), 
            imageCount, 
            swapChainFormat,
            vk::ColorSpaceKHR::eSrgbNonlinear, 
            extent, 
            1, 
            vk::ImageUsageFlagBits::eColorAttachment,
            sharingModeUtil.sharingMode, 
            sharingModeUtil.familyIndicesCount,
            sharingModeUtil.familyIndicesDataPtr, 
            vk::SurfaceTransformFlagBitsKHR::eIdentity,
            vk::CompositeAlphaFlagBitsKHR::eOpaque, 
            vk::PresentModeKHR::eFifo, 
            true, 
            nullptr
        };

        swapChain = device->createSwapchainKHRUnique(swapChainCreateInfo);
    }

    void getSwapchainImages() {
        swapChainImages = device->getSwapchainImagesKHR(swapChain.get());
        swapChainImageViews.reserve(swapChainImages.size());

        for (vk::Image image : swapChainImages) {
            vk::ImageViewCreateInfo imageViewCreateInfo = {
                vk::ImageViewCreateFlags(), 
                image,
                vk::ImageViewType::e2D, 
                swapChainFormat,
                vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
                vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } // No mips for swapchain imageviews
            };
            swapChainImageViews.push_back(device->createImageViewUnique(imageViewCreateInfo));
        }
    }

    void createShaderModules() {
        // TODO: Hardcoded for now
        std::string vertexShaderSource = R"vertexshader(
            #version 450
            #extension GL_ARB_separate_shader_objects : enable

            out gl_PerVertex {
                vec4 gl_Position;
            };

            layout(location = 0) out vec3 fragColor;

            vec2 positions[3] = vec2[](
                vec2(0.0, -0.5),
                vec2(0.5, 0.5),
                vec2(-0.5, 0.5)
            );

            vec3 colors[3] = vec3[](
                vec3(1.0, 0.0, 0.0),
                vec3(0.0, 1.0, 0.0),
                vec3(0.0, 0.0, 1.0)
            );

            void main() {
                gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
                fragColor = colors[gl_VertexIndex];
            }
            )vertexshader";

        std::string fragmentShaderSource = R"fragmentShader(
            #version 450
            #extension GL_ARB_separate_shader_objects : enable

            layout(location = 0) in vec3 fragColor;

            layout(location = 0) out vec4 outColor;

            void main() {
                outColor = vec4(fragColor, 1.0);
            }
            )fragmentShader";

        shaderc::SpvCompilationResult vertexShaderModuleCompile = compileShader(vertexShaderSource, shaderc_glsl_vertex_shader, "vertex shader");
        std::vector<uint32_t> vertexShaderCode = { vertexShaderModuleCompile.cbegin(), vertexShaderModuleCompile.cend() };
        ptrdiff_t vertexShaderNumBytes = std::distance(vertexShaderCode.begin(), vertexShaderCode.end());
        vk::ShaderModuleCreateInfo vertexShaderCreateInfo = {
            {}, 
            vertexShaderNumBytes * sizeof(uint32_t),
            vertexShaderCode.data() 
        };
        vertexShaderModule = device->createShaderModuleUnique(vertexShaderCreateInfo);

        shaderc::SpvCompilationResult fragShaderModuleCompile = compileShader(fragmentShaderSource, shaderc_glsl_fragment_shader, "fragment shader");
        std::vector<uint32_t> fragmentShaderCode = { fragShaderModuleCompile.cbegin(), fragShaderModuleCompile.cend() };
        ptrdiff_t fragmentShaderNumBytes = std::distance(fragmentShaderCode.begin(), fragmentShaderCode.end());
        vk::ShaderModuleCreateInfo fragShaderCreateInfo = {
            {}, 
            fragmentShaderNumBytes * sizeof(uint32_t), 
            fragmentShaderCode.data() 
        };
        fragmentShaderModule = device->createShaderModuleUnique(fragShaderCreateInfo);
    }

    void initVulkan() {
        createInstance();
        createDebugMessenger();
        createSurface();
        initPhysicalDevice();
        findQueueFamilyIndices();
        createDevice();
        createSwapchain();
        getSwapchainImages();
        createShaderModules();

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
        std::cerr << "[EXCEPTION] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}