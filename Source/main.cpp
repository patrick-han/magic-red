#include "vulkan/vulkan.h"
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <shaderc/shaderc.hpp>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <vulkan/vk_enum_string_helper.h>
#include "Common/Debug.h"

#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>
#include <stdexcept>
#include <cstdlib>

#include "RootDir.h"

#include "Control/Control.h"
#include "Common/Log.h"
#include "Shader.h"
#include "Types.h"
#include "Mesh/Mesh.h"
#include "Wrappers/Buffer.h"
#include "Wrappers/Image.h"
#include "Material.h"
#include "Mesh/RenderMesh.h"
#include "VertexDescriptors.h"
#include "Common/Config.h"
#include "Scene/Scene.h"
#include "Pipeline/GraphicsPipeline.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

// Frame data
int frameNumber = 0;
float deltaTime = 0.0f; // Time between current and last frame
float lastFrame = 0.0f; // Time of last frame

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

    // VulkanMemoryAllocator (VMA)
    VmaAllocator vmaAllocator;

    // Instance
    std::vector<const char*> glfwExtensionsVector;
    std::vector<const char*> layers;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    // Surface and devices
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    // Queues
    size_t graphicsQueueFamilyIndex;
    size_t presentQueueFamilyIndex;
    std::set<uint32_t> uniqueQueueFamilyIndices;
    std::vector<uint32_t> FamilyIndices;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    // Images
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    AllocatedImage depthImage;
    VkImageView depthImageView;

    // Swapchain
    VkExtent2D swapChainExtent;
    VkSwapchainKHR swapChain;
    VkFormat swapChainFormat;
    uint32_t swapChainImageCount = 2; // Should probably request support for this, but it's probably fine
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    // Shaders
    // VkShaderModule vertexShaderModule;
    // VkShaderModule fragmentShaderModule;

    // Synchronization (per in-flight frame resources)
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> renderFences;
    uint32_t currentFrame = 0;

    // Renderpass
    VkRenderPass renderPass;

    // Graphics Pipeline
    // VkPipelineLayout meshPipelineLayout;
    // VkPipeline meshPipeline;

    // Framebuffers
    std::vector<VkFramebuffer> framebuffers;

    // Command Pools and Buffers
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers; // (per in-flight frame resource)

    // Resources
    DeletionQueue mainDeletionQueue;

    // // Scene
    // std::vector<RenderMesh> sceneRenderMeshes;
    // std::unordered_map<std::string, Material> sceneMaterialMap;
    // std::unordered_map<std::string, Mesh> sceneMeshMap;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Engine", nullptr, nullptr);
        glfwSetKeyCallback(window, key_callback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide cursor and capture it
	    glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
    }

    void createInstance() {
        // Specify application and engine info
        VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "Hello Triangle", VK_MAKE_API_VERSION(1, 0, 0, 0), "Magic Red", VK_MAKE_API_VERSION(1, 0, 0, 0), VK_API_VERSION_1_2};

        // Get extensions required by GLFW for VK surface rendering. Should contain atleast VK_KHR_surface
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        glfwExtensionsVector = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // MoltenVK requires
        VkInstanceCreateFlagBits instanceCreateFlagBits = {};
        if (appleBuild) {
            std::cout << "Running on an Apple device, adding appropriate extension and instance creation flag bits" << std::endl;
            glfwExtensionsVector.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            instanceCreateFlagBits = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
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
        VkInstanceCreateInfo instanceCreateInfo = {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr,
            instanceCreateFlagBits,
            &appInfo,
            static_cast<uint32_t>(layers.size()),
            layers.data(),
            static_cast<uint32_t>(glfwExtensionsVector.size()),
            glfwExtensionsVector.data()
        };
        VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Failed to create instance!");
        }
        mainDeletionQueue.push_function([=]() {
            vkDestroyInstance(instance, nullptr);
        });
    }

    void createDebugMessenger() {
        if (!enableValidationLayers) {
            return;
        }
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
        debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugMessengerCreateInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerCreateInfo.pfnUserCallback = debugCallback;
        debugMessengerCreateInfo.pUserData = nullptr;

        VkResult res = CreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger);
        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Failed to create debug messenger!");
        }
        mainDeletionQueue.push_function([=]() {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        });
    }

    void createSurface() {
        VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Could not create surface!");
        }
        mainDeletionQueue.push_function([=]() {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        });
    }

    void initPhysicalDevice() {
        
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if (physicalDeviceCount == 0) {
            throw std::runtime_error("No Vulkan capable devices found");
        }
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

        for (VkPhysicalDevice d : physicalDevices) {
            VkPhysicalDeviceProperties physDeviceProps;
            vkGetPhysicalDeviceProperties(d, &physDeviceProps);
            MRLOG("Physical device enumerated: " << physDeviceProps.deviceName);
        }

        // Find Apple device if Apple build, otherwise just choose the first device in the list
        std::vector<VkPhysicalDevice>::iterator findIfIterator;
        if (appleBuild) {
            findIfIterator = std::find_if(physicalDevices.begin(), physicalDevices.end(), 
                [](const VkPhysicalDevice& physicalDevice) -> char* {
                    VkPhysicalDeviceProperties physDeviceProps;
                    vkGetPhysicalDeviceProperties(physicalDevice, &physDeviceProps);
                    return strstr(physDeviceProps.deviceName, "Apple"); // Returns null pointer if not found
                }
            );
            physicalDevice = physicalDevices[std::distance(physicalDevices.begin(), findIfIterator)];
            VkPhysicalDeviceProperties chosenPhysDeviceProps;
            vkGetPhysicalDeviceProperties(physicalDevice, &chosenPhysDeviceProps);
            MRLOG("Chosen deviceName: " << chosenPhysDeviceProps.deviceName);
        } else {
            physicalDevice = physicalDevices[0]; // TODO: By Default, just select the first physical device, could be bad if there are both integrated and discrete GPUs in the system
        }
    }

    void findQueueFamilyIndices() {
        // Find graphics and present queue family indices
        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

        graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(),
            std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                [](VkQueueFamilyProperties const& qfp) {
                    return qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT;
                }
            )
        );
        presentQueueFamilyIndex = 0u;
        for (unsigned long queueFamilyIndex = 0ul; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++) {
            // Check if a given queue family on our device supports presentation to the surface that was created
            VkBool32 supported = VK_FALSE;
            VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, static_cast<uint32_t>(queueFamilyIndex), surface, &supported);
            if (res != VK_SUCCESS) {
                MRCERR(string_VkResult(res));
                throw std::runtime_error("Queue family does not support presentation!");
            } else {
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
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 0.0f;
        uint32_t queueCountPerFamily = 1;
        for (auto& queueFamilyIndex : uniqueQueueFamilyIndices) {
            queueCreateInfos.push_back(VkDeviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, VkDeviceQueueCreateFlags(), static_cast<uint32_t>(queueFamilyIndex), queueCountPerFamily, &queuePriority });
        }

        // Device extensions
        std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        if (appleBuild) {
            deviceExtensions.push_back("VK_KHR_portability_subset");
        }
        

        VkDeviceCreateInfo deviceCreateInfo = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            nullptr,
            VkDeviceCreateFlags(),
            static_cast<uint32_t>(queueCreateInfos.size()),
            queueCreateInfos.data(),
            0u,
            nullptr,
            static_cast<uint32_t>(deviceExtensions.size()),
            deviceExtensions.data()
        };
        vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
        mainDeletionQueue.push_function([=]() {
            vkDestroyDevice(device, nullptr);
        });
    }

    void createSwapchain() {
        // If the graphics and presentation queue family indices are different, allow concurrent access to object from multiple queue families
        struct SM {
            VkSharingMode sharingMode;
            uint32_t familyIndicesCount;
            uint32_t* familyIndicesDataPtr;
        } sharingModeUtil  = { (graphicsQueueFamilyIndex != presentQueueFamilyIndex) ?
                           SM{ VK_SHARING_MODE_CONCURRENT, 2u, FamilyIndices.data() } :
                           SM{ VK_SHARING_MODE_EXCLUSIVE, 0u, static_cast<uint32_t*>(nullptr) } };
        
        // Query for surface format support
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, formats.data());
        
        swapChainFormat = VK_FORMAT_B8G8R8A8_UNORM;
        VkColorSpaceKHR swapChainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        bool foundCompatible = false;
        for (auto format : formats) {
            if (format.format == swapChainFormat && format.colorSpace == swapChainColorSpace) {
                foundCompatible = true;
                break;
            }
        }
        if (!foundCompatible) {
            throw std::runtime_error("Could not find compatible surface format!");
        }

        // Create swapchain
        swapChainExtent = VkExtent2D{ WINDOW_WIDTH, WINDOW_HEIGHT };
        VkSwapchainCreateInfoKHR swapChainCreateInfo = {
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            nullptr,
            {},
            surface, 
            swapChainImageCount, // This is a minimum, not exact
            swapChainFormat,
            swapChainColorSpace,
            swapChainExtent, 
            1, 
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            sharingModeUtil.sharingMode,
            sharingModeUtil.familyIndicesCount,
            sharingModeUtil.familyIndicesDataPtr,
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_PRESENT_MODE_FIFO_KHR, // Required for implementations, no need to query for support
            true, 
            nullptr
        };
        VkResult res = vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain);
        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Could not create swap chain!");
        }
        mainDeletionQueue.push_function([=]() {
            vkDestroySwapchainKHR(device, swapChain, nullptr);
        });
    }

    void getSwapchainImages() {
        vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr); // We only specified a minimum during creation, need to query for the real number
        swapChainImages.resize(swapChainImageCount); 
        vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());
        swapChainImageViews.resize(swapChainImages.size());

        for (int i = 0; i < swapChainImageViews.size(); i++) {
            VkImageViewCreateInfo imageViewCreateInfo = {
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                nullptr,
                VkImageViewCreateFlags(),
                swapChainImages[i],
                VK_IMAGE_VIEW_TYPE_2D,
                swapChainFormat,
                VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } // No mips for swapchain imageviews
            };
            VkResult res = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]);
            if (res != VK_SUCCESS) {
                MRCERR(string_VkResult(res));
                throw std::runtime_error("Could not create swap chain image view!");
            }
        }
        mainDeletionQueue.push_function([=]() {
            for (int k = 0; k < swapChainImageViews.size(); k++) {
                vkDestroyImageView(device, swapChainImageViews[k], nullptr);
            }   
        });
    }

    void createDepthImageAndView() {
        VkExtent3D depthExtent = VkExtent3D{ swapChainExtent.width, swapChainExtent.height, 1 };

        VkImageCreateInfo depthImageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, VkImageCreateFlags(), VK_IMAGE_TYPE_2D, depthFormat, depthExtent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, VK_IMAGE_LAYOUT_UNDEFINED};

        VkImageCreateInfo ci = static_cast<VkImageCreateInfo>(depthImageCreateInfo);

        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; 
        VkResult res = vmaCreateImage(vmaAllocator, &ci, &vmaAllocInfo, &reinterpret_cast<VkImage &>(depthImage.image), &depthImage.allocation, nullptr);

        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Could not create depth image!");
        }

        VkImageSubresourceRange depthSubresRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        VkImageViewCreateInfo depthImageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, VkImageViewCreateFlags(), depthImage.image, VK_IMAGE_VIEW_TYPE_2D, depthFormat, {}, depthSubresRange};
        vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthImageView);

        mainDeletionQueue.push_function([=]() {
            vkDestroyImageView(device, depthImageView, nullptr);
            vmaDestroyImage(vmaAllocator, depthImage.image, depthImage.allocation);
        });
    }

    // void createShaderModules() {
    //     // TODO: Hardcoded for now
    //     std::string vertexShaderSource = load_shader_source_to_string(ROOT_DIR "Shaders/triangle_mesh.vert");
    //     std::string fragmentShaderSource = load_shader_source_to_string(ROOT_DIR "Shaders/triangle_mesh.frag");

    //     compile_shader(device, vertexShaderModule, vertexShaderSource, shaderc_glsl_vertex_shader, "vertex shader");
    //     compile_shader(device, fragmentShaderModule, fragmentShaderSource, shaderc_glsl_fragment_shader, "fragment shader");

    //      mainDeletionQueue.push_function([=]() {
    //         vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    //         vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    //     });

    // }

    void createSynchronizationStructures() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFences.resize(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, {}, {}};
            vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]);
            vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);
            mainDeletionQueue.push_function([=]() {
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            });

            VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
            vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFences[i]);
            mainDeletionQueue.push_function([=]() {
                vkDestroyFence(device, renderFences[i], nullptr);
            });
        }
    }

    void createRenderPass() {
        // Prepare attachment descriptions and references
        VkAttachmentDescription colorAttachment = { VkAttachmentDescriptionFlags(), swapChainFormat, VK_SAMPLE_COUNT_1_BIT, 
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            {}, // Stencil Load
            {}, // Stencil Store
            {}, // Initial Layout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // Final layout
        };
        VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkAttachmentDescription depthAttachment = { VkAttachmentDescriptionFlags(), depthFormat, VK_SAMPLE_COUNT_1_BIT, 
            VK_ATTACHMENT_LOAD_OP_CLEAR,      // Color/depth
            VK_ATTACHMENT_STORE_OP_STORE,     // Color/depth
            VK_ATTACHMENT_LOAD_OP_CLEAR,      // Stencil
            VK_ATTACHMENT_STORE_OP_DONT_CARE, // Stencil
            VK_IMAGE_LAYOUT_UNDEFINED,                       // Renderpass instance begin layout
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // Renderpass instance end layout
        };
        VkAttachmentReference depthAttachmentRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }; // Layout during the subpass

        VkSubpassDescription subpass = {
            VkSubpassDescriptionFlags(), 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            0,                  // Input attachment count
            nullptr, 
            1, 
            &colorAttachmentRef, 
            {},                 // Resolve attachments
            &depthAttachmentRef, 
            0,
            nullptr,
        };

        // Color attachment synchronization
        VkSubpassDependency subpassDependencyColor = { // TODO: Understand
            VK_SUBPASS_EXTERNAL,                               // srcSubpass
            0,                                                 // dstSubpass
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask This should wait for swapchain to finish reading from the image
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
            {},                                                // srcAccessMask
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,         // dstAccessMask
            {} // dependencyFlags
        };

        // Depth attachment synchronization
        VkSubpassDependency subpassDependencyDepth = { // TODO: Understand
            VK_SUBPASS_EXTERNAL,
            0,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // Potentially used in either
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            {},
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            {} // dependencyFlags
        };

        VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
        VkSubpassDependency subpassDependencies[2] = { subpassDependencyColor, subpassDependencyDepth };

        VkRenderPassCreateInfo rpCreateInfo = {};
        rpCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpCreateInfo.pNext = nullptr;
        rpCreateInfo.flags = {};
        rpCreateInfo.attachmentCount = 2;
        rpCreateInfo.pAttachments = attachments;
        rpCreateInfo.subpassCount = 1;
        rpCreateInfo.pSubpasses = &subpass;
        rpCreateInfo.dependencyCount = 2;
        rpCreateInfo.pDependencies = subpassDependencies;


        vkCreateRenderPass(device, &rpCreateInfo, nullptr, &renderPass);
        mainDeletionQueue.push_function([=]() {
            vkDestroyRenderPass(device, renderPass, nullptr);
        });
    }

    // void createGraphicsPipeline() {
    //     VkPipelineShaderStageCreateInfo vertShaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, VkPipelineShaderStageCreateFlags(), VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main", nullptr};
    //     VkPipelineShaderStageCreateInfo fragShaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, VkPipelineShaderStageCreateFlags(), VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main", nullptr};

    //     std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStages = { vertShaderStageInfo, fragShaderStageInfo };
        
    //     VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, VkPipelineVertexInputStateCreateFlags(), 0u, nullptr, 0u, nullptr };
    //     VertexInputDescription vertexDescription = VertexInputDescription::get_default_vertex_description();
    //     vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
    //     vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    //     vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
    //     vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
        
    //     VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, VkPipelineInputAssemblyStateCreateFlags(), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE };
    //     VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT), 0.0f, 1.0f };
    //     VkRect2D scissor = { { 0, 0 }, swapChainExtent };
    //     VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, VkPipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor };
    //     VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, VkPipelineRasterizationStateCreateFlags(), /*depthClamp*/ VK_FALSE,
    //     /*rasterizeDiscard*/ VK_FALSE, VK_POLYGON_MODE_FILL, VkCullModeFlags(),
    //     /*frontFace*/ VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, {}, {}, {}, 1.0f };

    //     VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, VkPipelineDepthStencilStateCreateFlags(),
    //         bDepthTest ? VK_TRUE : VK_FALSE,
    //         bDepthWrite ? VK_TRUE : VK_FALSE,
    //         VK_COMPARE_OP_LESS_OR_EQUAL,
    //         VK_FALSE, // depth bounds test
    //         VK_FALSE, // stencil
    //         {}, {}, {}, {}
    //     };

    //     VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, VkPipelineMultisampleStateCreateFlags(), VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.0 , {}, {}, {}};

    //     VkPipelineColorBlendAttachmentState colorBlendAttachment = { VK_FALSE, /*srcCol*/ VK_BLEND_FACTOR_ONE,
    //     /*dstCol*/ VK_BLEND_FACTOR_ZERO, /*colBlend*/ VK_BLEND_OP_ADD,
    //     /*srcAlpha*/ VK_BLEND_FACTOR_ONE, /*dstAlpha*/ VK_BLEND_FACTOR_ZERO,
    //     /*alphaBlend*/ VK_BLEND_OP_ADD,
    //     VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    //         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };
    //     VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, VkPipelineColorBlendStateCreateFlags(), /*logicOpEnable=*/false, VK_LOGIC_OP_COPY, /*attachmentCount=*/1, /*colourAttachments=*/&colorBlendAttachment, {}};

    //     std::vector<VkDynamicState> dynamicStates = {
    //         VK_DYNAMIC_STATE_VIEWPORT,
    //         VK_DYNAMIC_STATE_SCISSOR
    //     };
    //     VkPipelineDynamicStateCreateInfo dynamicState = {};
    //     dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //     dynamicState.pNext = nullptr;
    //     dynamicState.flags = VkPipelineDynamicStateCreateFlags();
    //     dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    //     dynamicState.pDynamicStates = dynamicStates.data();

    //     // Push constants
    //     VkPushConstantRange meshPushConstant = {};
    //     meshPushConstant.offset = 0;
    //     meshPushConstant.size = sizeof(MeshPushConstants);
    //     meshPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //     VkPipelineLayoutCreateInfo meshPipelineLayoutCreateInfo = {};
    //     meshPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    //     meshPipelineLayoutCreateInfo.pNext = nullptr;
    //     meshPipelineLayoutCreateInfo.flags = {};
    //     meshPipelineLayoutCreateInfo.setLayoutCount = 0;
    //     meshPipelineLayoutCreateInfo.pSetLayouts = nullptr;
    //     meshPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    //     meshPipelineLayoutCreateInfo.pPushConstantRanges = &meshPushConstant;

    //     vkCreatePipelineLayout(device, &meshPipelineLayoutCreateInfo, nullptr, &meshPipelineLayout);
    //     mainDeletionQueue.push_function([=]() {
    //         vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
    //     });

    //     VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
    //         VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    //         nullptr,
    //         VkPipelineCreateFlags(),
    //         2, 
    //         pipelineShaderStages.data(), 
    //         &vertexInputInfo, 
    //         &inputAssembly, 
    //         nullptr, 
    //         &viewportState, 
    //         &rasterizer, 
    //         &multisampling,
    //         &depthStencil,
    //         &colorBlending,
    //         &dynamicState, // Dynamic State
    //         meshPipelineLayout,
    //         renderPass,
    //         0,
    //         {},
    //         0
    //     };

    //     vkCreateGraphicsPipelines(device, {}, 1, &pipelineCreateInfo, nullptr, &meshPipeline);
    //     mainDeletionQueue.push_function([=]() {
    //         vkDestroyPipeline(device, meshPipeline, nullptr);
    //     });

    //     create_material(meshPipeline, meshPipelineLayout, "defaultMesh", sceneMaterialMap);
    // }
    
    //temp
    struct MeshPushConstants {
            glm::vec4 data;
            glm::mat4 renderMatrix;

            static VkPushConstantRange range() {
                VkPushConstantRange defaultPushConstantRange = {};
                defaultPushConstantRange.offset = 0;
                defaultPushConstantRange.size = sizeof(MeshPushConstants);
                defaultPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                return defaultPushConstantRange;
            }
     };

    std::vector<VkPushConstantRange> defaultPushConstantRanges = {MeshPushConstants::range()};

    GraphicsPipeline defaultPipeline;
    void createPipelines() {
        
        // std::vector<VkPushConstantRange> defaultPushConstantRanges = {MeshPushConstants::range()};
        
        // defaultPushConstanRanges.push_back(MeshPushConstants::range());

        defaultPipeline = GraphicsPipeline(device, renderPass, std::string("Shaders/triangle_mesh.vert"), std::string("Shaders/triangle_mesh.frag"), defaultPushConstantRanges);
        mainDeletionQueue.push_function([&]() {
		    defaultPipeline.Destroy();
        });

        Scene::GetInstance()->sceneGraphicsPipelines.push_back(defaultPipeline);

        // create_material(&Scene::GetInstance()->sceneGraphicsPipelines[0], "defaultMaterial", Scene::GetInstance()->sceneMaterialMap);
        create_material(&defaultPipeline, "defaultMaterial", Scene::GetInstance()->sceneMaterialMap);
    }

    void createFramebuffers() {
        framebuffers.resize(swapChainImageCount);

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[2] = { swapChainImageViews[i], depthImageView};
            VkFramebufferCreateInfo framebufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, VkFramebufferCreateFlags(), renderPass, 2, attachments, swapChainExtent.width, swapChainExtent.height, 1 };

            VkResult res = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffers[i]);
            if (res != VK_SUCCESS) {
                MRCERR(string_VkResult(res));
                throw std::runtime_error("Framebuffer creation unccessful!");
            }
            mainDeletionQueue.push_function([=]() {
                vkDestroyFramebuffer(device, framebuffers[i], nullptr);
            });
        }
        MRLOG("Framebuffers size: " << framebuffers.size());
        MRLOG("Framebuffers capacity: " << framebuffers.capacity());
        // for (auto fb : framebuffers) {
        //     if (fb == VK_NULL_HANDLE) {
        //         MRLOG("NULL HANDLE FRAMEBUFFER!");
        //     } else {
        //         MRLOG("ALL GOOD");
        //     }
        // }
    }

    void createCommandPool() {
        VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer.
        static_cast<uint32_t>(graphicsQueueFamilyIndex)};
        vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

        mainDeletionQueue.push_function([=]() {
            vkDestroyCommandPool(device, commandPool, nullptr);
        });
    }
    

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
        cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferAllocInfo.commandPool = commandPool;
        cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, commandBuffers.data());
    }

    void retrieveQueues() {
        vkGetDeviceQueue(device, static_cast<uint32_t>(graphicsQueueFamilyIndex), 0, &graphicsQueue);
        vkGetDeviceQueue(device, static_cast<uint32_t>(presentQueueFamilyIndex), 0, &presentQueue);
    }

    void initVMA() {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        VkResult res = vmaCreateAllocator(&allocatorInfo, &vmaAllocator);
        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("vmaCreateAllocator unsuccessful!");
        }
        mainDeletionQueue.push_function([&]() {
		    vmaDestroyAllocator(vmaAllocator);
        });
    }

    void load_meshes() {
        Mesh triangleMesh;
        triangleMesh.vertices.resize(4);
        triangleMesh.vertices[0].position = { 1.f, 1.f, 0.0f }; // Lower right
        triangleMesh.vertices[1].position = {-1.f, 1.f, 0.0f }; // Lower left
        triangleMesh.vertices[2].position = { 1.f,-1.f, 0.0f }; // Upper right
        triangleMesh.vertices[3].position = {-1.f,-1.f, 0.0f }; // Upper left

        // Don't care about normals for now

        triangleMesh.vertices[0].color = { 1.f, 0.f, 0.f}; // Red
        triangleMesh.vertices[1].color = { 0.f, 1.f, 0.f}; // Green
        triangleMesh.vertices[2].color = { 0.f, 0.f, 1.f}; // Blue
        triangleMesh.vertices[3].color = { 1.f, 1.f, 0.f}; // Yellow

        triangleMesh.indices.resize(6);
        triangleMesh.indices.push_back(3);
        triangleMesh.indices.push_back(2);
        triangleMesh.indices.push_back(0);
        triangleMesh.indices.push_back(0);
        triangleMesh.indices.push_back(3);
        triangleMesh.indices.push_back(1);

         Scene::GetInstance()->sceneMeshMap["triangle"] = upload_mesh(triangleMesh, vmaAllocator, mainDeletionQueue);
        
        // Suzanne mesh
        Mesh monkeyMesh;
        load_mesh_from_obj(monkeyMesh, ROOT_DIR "/Assets/Meshes/suzanne.obj");
         Scene::GetInstance()->sceneMeshMap["suzanne"] = upload_mesh(monkeyMesh, vmaAllocator, mainDeletionQueue);

        // Sponza mesh
        Mesh sponzaMesh;
        load_mesh_from_obj(sponzaMesh, ROOT_DIR "/Assets/Meshes/sponza.obj");
         Scene::GetInstance()->sceneMeshMap["sponza"] = upload_mesh(sponzaMesh, vmaAllocator, mainDeletionQueue);
    }

    void init_scene() {
        RenderMesh sponzaObject;
        sponzaObject.material = get_material("defaultMaterial",  Scene::GetInstance()->sceneMaterialMap);
        sponzaObject.mesh = get_mesh("sponza",  Scene::GetInstance()->sceneMeshMap);
        glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, -5.0f, 0.0f));
        glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.05f, 0.05f, 0.05f));
        sponzaObject.transformMatrix = translate * scale;

        Scene::GetInstance()->sceneRenderMeshes.push_back(sponzaObject);

        RenderMesh monkeyObject;
        monkeyObject.material = get_material("defaultMaterial",  Scene::GetInstance()->sceneMaterialMap);
        monkeyObject.mesh = get_mesh("suzanne",  Scene::GetInstance()->sceneMeshMap);
        glm::mat4 monkeyTranslate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
        monkeyObject.transformMatrix = monkeyTranslate;

         Scene::GetInstance()->sceneRenderMeshes.push_back(monkeyObject);

        for (int x = -10; x < 10; x++) {
            for (int y = -10; y < 10; y++) {
                RenderMesh triangleObject;
                triangleObject.material = get_material("defaultMaterial",  Scene::GetInstance()->sceneMaterialMap);
                triangleObject.mesh = get_mesh("triangle",  Scene::GetInstance()->sceneMeshMap);
                glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(x, 0.0f, y));
                glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
                triangleObject.transformMatrix = translate * scale;
                
                 Scene::GetInstance()->sceneRenderMeshes.push_back(triangleObject);
            }
        }
    }

    void draw_objects(uint32_t imageIndex) {
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 200.0f);
        projection[1][1] *= -1; // flips the model because Vulkan uses positive Y downwards


        for (RenderMesh renderObject :  Scene::GetInstance()->sceneRenderMeshes) {
            
            vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, (*renderObject.material->pipeline).getPipeline());


            VkDeviceSize offset = 0;
            
            vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, &renderObject.mesh->vertexBuffer.buffer, &offset);

            VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f };
            vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

            VkRect2D scissor = { {0, 0}, swapChainExtent};
            vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

            // Compute MVP matrix
            glm::mat4 model = renderObject.transformMatrix;
            // glm::mat4 rotate = glm::rotate(glm::mat4{ 1.0f }, glm::radians(frameNumber * 0.4f), glm::vec3(0, 1, 0));
            // model = rotate * model;
            glm::mat4 mvpMatrix = projection * view * model;

            MeshPushConstants constants;
            constants.renderMatrix = mvpMatrix;
            vkCmdPushConstants(commandBuffers[currentFrame], (*renderObject.material->pipeline).getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
            vkCmdDraw(commandBuffers[currentFrame], renderObject.mesh->vertices.size(), 1, 0, 0);
        }
    }

    void initVulkan() {
        createInstance();
        createDebugMessenger();
        createSurface();
        initPhysicalDevice();
        findQueueFamilyIndices();
        createDevice();
        initVMA();
        createSwapchain();
        getSwapchainImages();
        createDepthImageAndView();
        // createShaderModules();
        createSynchronizationStructures();
        createRenderPass();
        createPipelines();
        // createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        retrieveQueues();
        load_meshes();
        init_scene();

    }

    void drawFrame() {
            // Wait for previous frame to finish rendering before allowing us to acquire another image
            
            VkResult res = vkWaitForFences(device, 1, &renderFences[currentFrame], true, (std::numeric_limits<uint64_t>::max)());
            vkResetFences(device, 1, &renderFences[currentFrame]);

            uint32_t imageIndex;
            res = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
            if (!((res == VK_SUCCESS) || (res == VK_SUBOPTIMAL_KHR))) {
                MRCERR(string_VkResult(res));
                throw std::runtime_error("Failed to acquire image from Swap Chain!");
            }
            vkResetCommandBuffer(commandBuffers[currentFrame], {});

            VkClearValue clearValue = {{0.0f, 0.0f, 0.5f, 1.0f}};
            VkClearValue clearValueDepth = {{1.0f, 0}};
            VkClearValue clearValues[2] = { clearValue, clearValueDepth };

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.framebuffer = framebuffers[imageIndex];
            renderPassBeginInfo.renderArea = VkRect2D{ {0, 0}, swapChainExtent};
            renderPassBeginInfo.clearValueCount = 2;
            renderPassBeginInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            draw_objects(imageIndex);
            vkCmdEndRenderPass(commandBuffers[currentFrame]);
            vkEndCommandBuffer(commandBuffers[currentFrame]);


            // Submit graphics workload
            VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
            submitInfo.pWaitDstStageMask = &waitStageMask;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFences[currentFrame]);


            // Present frame
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain;
            presentInfo.pImageIndices = &imageIndex;
            res = vkQueuePresentKHR(presentQueue, &presentInfo);


            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            frameNumber++;
            
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            process_input(window, deltaTime);

            drawFrame();
        }
    }

    void cleanup() {
        // device->waitIdle();
        vkDeviceWaitIdle(device);

        mainDeletionQueue.flush();

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