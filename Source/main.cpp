#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vulkan/vk_enum_string_helper.h> // Doesn't work on linux?
#include <Common/Debug.h>

#include <vector>
#include <set>
#include <stdexcept>
#include <cstdlib>

#include <Common/RootDir.h>
#include <Common/Platform.h>

#include <Control/Camera.h>
#include <Common/Log.h>
#include <DeletionQueue.h>
#include <Mesh/Mesh.h>

#include <Wrappers/Buffer.h>
#include <Wrappers/Image.h>
#include <Wrappers/ImageMemoryBarrier.h>
#include <Wrappers/DynamicRendering.h>

#include <Pipeline/MaterialFunctions.h>
#include <Mesh/RenderObject.h>
#include <Vertex/VertexDescriptors.h>
#include <Common/Config.h>
#include <Common/Defaults.h>
#include <Scene/Scene.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Mesh/MeshPushConstants.h>
#include <Descriptor/Descriptor.h>

#include <IncludeHelpers/ImguiIncludes.h>

// NVRHI
#include <nvrhi/vulkan.h>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
// Frame data
int frameNumber = 0;
float deltaTime = 0.0f; // Time between current and last frame
uint64_t lastFrameTick = 0;
uint64_t currentFrameTick = 0;
bool interactableUI = false;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -2.0f);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
Camera camera(cameraPos, worldUp, cameraFront, -90.0f, 0.0f, 45.0f, true);
float cameraSpeed = 0.0f;
bool firstMouse = true;
float lastX = WINDOW_WIDTH / 2, lastY = WINDOW_HEIGHT / 2; // Initial mouse positions

class Engine {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    SDL_Window *window;

    // VulkanMemoryAllocator (VMA)
    VmaAllocator vmaAllocator;

    // Instance
    std::vector<const char*> extensionsVector;
    std::vector<const char*> layers;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    // Surface and devices
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    // NVRHI
    nvrhi::DeviceHandle nvrhiDevice;
    nvrhi::IMessageCallback* nvrhiMessageCallback;

    // Queues
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;
    std::set<uint32_t> uniqueQueueFamilyIndices;
    std::vector<uint32_t> FamilyIndices;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    // Images
    AllocatedImage depthImage;
    AllocatedImage drawImage;
    // VkExtent2D drawExtent; // Allow for resizing

    // Swapchain
    VkSwapchainKHR swapChain;
    VkFormat swapChainFormat;
    uint32_t swapChainImageCount = 3; // Should probably request support for this, but it's probably fine
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores_F;
    std::vector<VkSemaphore> renderFinishedSemaphores_F;
    std::vector<VkFence> renderFences_F;
    uint32_t currentFrame = 0;

    // Command Pools and Buffers
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers_F;

    // Descriptors
    DescriptorAllocator globalDescriptorAllocator;
    std::vector<VkDescriptorSetLayout> sceneDescriptorSetLayouts;
    std::vector<VkDescriptorSet> sceneDescriptorSets_F;

    // Lights
    std::vector<AllocatedBuffer> PointLightsBuffers_F;

    // Immediate rendering resources
    VkFence immediateFence;
    VkCommandBuffer immediateCommandBuffer;
    VkCommandPool immediateCommandPool;

    // Cleanup
    DeletionQueue mainDeletionQueue; // Contains all deletable vulkan resources except pipelines/pipeline layouts

    void initWindow() {
        // We initialize SDL and create a window with it.
        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow("Magic Red", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    void createInstance() {
        // Specify application and engine info
        VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "Magic Red", VK_MAKE_API_VERSION(1, 0, 0, 0), "Magic Red", VK_MAKE_API_VERSION(1, 0, 0, 0), VK_API_VERSION_1_2};

        // Get extensions required for SDL VK surface rendering
        uint32_t sdlExtensionCount = 0;
        SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        extensionsVector.resize(sdlExtensionCount);
        const char* const* sdlVulkanExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        for (uint32_t i = 0; i < sdlExtensionCount; i++) {
            extensionsVector[i] = sdlVulkanExtensions[i];
        }
        

        // MoltenVK requires
        VkInstanceCreateFlags instanceCreateFlagBits = {};
#if PLATFORM_MACOS
            MRLOG("Running on an Apple device, adding appropriate extension and instance creation flag bits");
            extensionsVector.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            instanceCreateFlagBits |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        // Enable validation layers and creation of debug messenger if building debug
        if (enableValidationLayers) {
            MRLOG("Debug build");
            extensionsVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
            static_cast<uint32_t>(extensionsVector.size()),
            extensionsVector.data()
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
        SDL_bool res = SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);
        if (res != SDL_TRUE) {
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
#if PLATFORM_MACOS
        // Find Apple device if Apple build, otherwise just choose the first device in the list
        std::vector<VkPhysicalDevice>::iterator findIfIterator;
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
#else
            physicalDevice = physicalDevices[0]; // TODO: By Default, just select the first physical device, could be bad if there are both integrated and discrete GPUs in the system
#endif
    }

    void findQueueFamilyIndices() {
        // Find graphics and present queue family indices
        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

        graphicsQueueFamilyIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(),
            std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                [](VkQueueFamilyProperties const& qfp) {
                    return qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT;
                }
            )
        ));
        presentQueueFamilyIndex = 0u;
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++) {
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
            graphicsQueueFamilyIndex,
            presentQueueFamilyIndex
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
        std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_dynamic_rendering" };
#if PLATFORM_MACOS
        deviceExtensions.push_back("VK_KHR_portability_subset");
#endif
        
        // Needed to enable dynamic rendering extension
        constexpr VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
            .dynamicRendering = VK_TRUE,
        };

        VkDeviceCreateInfo deviceCreateInfo = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            &dynamic_rendering_feature,
            VkDeviceCreateFlags(),
            static_cast<uint32_t>(queueCreateInfos.size()),
            queueCreateInfos.data(),
            0u,
            nullptr,
            static_cast<uint32_t>(deviceExtensions.size()),
            deviceExtensions.data(),
            nullptr
        };
        vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
        mainDeletionQueue.push_function([=]() {
            vkDestroyDevice(device, nullptr);
        });

        ////nvrhiMessageCallback = new MessageCallbackImpl;
        nvrhi::vulkan::DeviceDesc deviceDesc;
        // deviceDesc.errorCB = nvrhiMessageCallback;
        deviceDesc.errorCB = nullptr;
        deviceDesc.physicalDevice = physicalDevice;
        deviceDesc.device = device;
        deviceDesc.graphicsQueue = graphicsQueue;
        deviceDesc.graphicsQueueIndex = graphicsQueueFamilyIndex;
        deviceDesc.deviceExtensions = deviceExtensions.data();
        deviceDesc.numDeviceExtensions = std::size(deviceExtensions);
        nvrhi::vulkan::createDevice(deviceDesc);
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
        VkExtent2D swapChainExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };
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
        MRLOG("Final number of swapchain images: " << swapChainImageCount);
        assert(swapChainImageCount >= MAX_FRAMES_IN_FLIGHT); // Need at least as many swapchain images as FiFs or the extra FiFs are useless
        swapChainImageViews.resize(swapChainImages.size());

        for (uint32_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageViewCreateInfo imageViewCreateInfo = imageview_create_info(swapChainImages[i], swapChainFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
            VkResult res = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]);
            if (res != VK_SUCCESS) {
                MRCERR(string_VkResult(res));
                throw std::runtime_error("Could not create swap chain image view!");
            }
        }
        mainDeletionQueue.push_function([=]() {
            for (uint32_t k = 0; k < swapChainImageViews.size(); k++) {
                vkDestroyImageView(device, swapChainImageViews[k], nullptr);
            }   
        });
    }

    void createDrawImage() {
        VkExtent3D drawImageExtent = {
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            1
        };
        drawImage.imageExtent = drawImageExtent;
        drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        
        VkImageUsageFlags drawImageUsages = {};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        VkImageCreateInfo drawImageCreateInfo = image_create_info(drawImage.imageFormat, drawImage.imageExtent, drawImageUsages);

        upload_gpu_only_image(drawImage, drawImageCreateInfo, vmaAllocator, mainDeletionQueue);

        VkImageViewCreateInfo drawImageViewCreateInfo = imageview_create_info(drawImage.image, drawImage.imageFormat, {}, VK_IMAGE_ASPECT_COLOR_BIT);
        VkResult res = vkCreateImageView(device, &drawImageViewCreateInfo, nullptr, &drawImage.imageView);
        if(res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Could not create draw image view!");
        }

        mainDeletionQueue.push_function([=]() {
            vkDestroyImageView(device, drawImage.imageView, nullptr);
        });
    }

    void createDepthImageAndView() {
        depthImage.imageExtent = VkExtent3D{ WINDOW_WIDTH, WINDOW_HEIGHT, 1 };
        depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;

        VkImageCreateInfo depthImageCreateInfo = image_create_info(depthImage.imageFormat, depthImage.imageExtent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; 
        VkResult res = vmaCreateImage(vmaAllocator, &depthImageCreateInfo, &vmaAllocInfo, &reinterpret_cast<VkImage &>(depthImage.image), &depthImage.allocation, nullptr);

        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Could not create depth image!");
        }

        VkImageViewCreateInfo depthImageViewCreateInfo = imageview_create_info(depthImage.image, depthImage.imageFormat, {},VK_IMAGE_ASPECT_DEPTH_BIT);

        vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthImage.imageView);

        mainDeletionQueue.push_function([=]() {
            vkDestroyImageView(device, depthImage.imageView, nullptr);
            vmaDestroyImage(vmaAllocator, depthImage.image, depthImage.allocation);
        });
    }

    void createSynchronizationStructures() {
        imageAvailableSemaphores_F.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores_F.resize(MAX_FRAMES_IN_FLIGHT);
        renderFences_F.resize(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, {}, {}};
            vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores_F[i]);
            vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores_F[i]);
            mainDeletionQueue.push_function([=]() {
                vkDestroySemaphore(device, imageAvailableSemaphores_F[i], nullptr);
                vkDestroySemaphore(device, renderFinishedSemaphores_F[i], nullptr);
            });

            VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
            vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFences_F[i]);
            mainDeletionQueue.push_function([=]() {
                vkDestroyFence(device, renderFences_F[i], nullptr);
            });
        }

        // Immediate submission synchronization structures
        {
            VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
            vkCreateFence(device, &fenceCreateInfo, nullptr, &immediateFence);
            mainDeletionQueue.push_function([=]() {
                vkDestroyFence(device, immediateFence, nullptr);
            });
        }
    }

    void createCommandPool() {
        VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer.
        graphicsQueueFamilyIndex};
        vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
        vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &immediateCommandPool);

        mainDeletionQueue.push_function([=]() {
            vkDestroyCommandPool(device, commandPool, nullptr);
            vkDestroyCommandPool(device, immediateCommandPool, nullptr);
        });
    }
    

    void createCommandBuffers() {
        // Main per FiF command buffers
        commandBuffers_F.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
        cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferAllocInfo.commandPool = commandPool;
        cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_F.size());
        vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, commandBuffers_F.data());

        // Immediate command buffer
        VkCommandBufferAllocateInfo immCmdBufferAllocInfo = {};
        immCmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        immCmdBufferAllocInfo.commandPool = immediateCommandPool;
        immCmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        immCmdBufferAllocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &immCmdBufferAllocInfo, &immediateCommandBuffer);
    }

    void retrieveQueues() {
        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
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

    void init_scene_lights() {
        Scene::GetInstance().scenePointLights.push_back(PointLight(glm::vec3(0.0f, 3.5f, -4.0f), glm::vec3(1.0f, 1.0f, 1.0f)));

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            AllocatedBuffer pointLightBuffer;
            PointLightsBuffers_F.push_back(pointLightBuffer);
            upload_buffer(
                PointLightsBuffers_F[i],
                Scene::GetInstance().scenePointLights.size() * sizeof(PointLight),
                Scene::GetInstance().scenePointLights.data(),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                vmaAllocator,
                mainDeletionQueue
            );
        }
    }
    
    void init_scene_descriptors() {
        // Describe what and how many descriptors we want and create our pool
        // These may be distributed in any combination among our sets
        std::vector<DescriptorAllocator::DescriptorTypeCount> descriptorTypeCounts = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT } // We want 1 buffer for each frame in flight
        };
        globalDescriptorAllocator.init_pool(device, MAX_FRAMES_IN_FLIGHT, descriptorTypeCounts); // We can allocate up to MAX_FRAMES_IN_FLIGHT sets from this pool

        DescriptorLayoutBuilder layoutBuilder;
        layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        // We only need a single layout since they are all the same for each frame in flight
        sceneDescriptorSetLayouts.push_back(layoutBuilder.buildLayout(device, VK_SHADER_STAGE_FRAGMENT_BIT));
        mainDeletionQueue.push_function([&]() {
                vkDestroyDescriptorSetLayout(device, sceneDescriptorSetLayouts[0], nullptr);
        });
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            sceneDescriptorSets_F.push_back(
                globalDescriptorAllocator.allocate(device, sceneDescriptorSetLayouts[0])
            );

            // Update descriptor set(s)
            VkDescriptorBufferInfo pointLightBufferInfo = {
                .buffer = PointLightsBuffers_F[i].buffer,
                .offset = 0,
                .range = Scene::GetInstance().scenePointLights.size() * sizeof(PointLight) // VK_WHOLE_SIZE?
            };
            VkWriteDescriptorSet pointLightDescriptorWrite = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = sceneDescriptorSets_F[i],
                .dstBinding = 0,
                .dstArrayElement = {},
                .descriptorCount = static_cast<uint32_t>(Scene::GetInstance().scenePointLights.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &pointLightBufferInfo,
                .pTexelBufferView = nullptr // ???
            };
            vkUpdateDescriptorSets(device, 1, &pointLightDescriptorWrite, 0, nullptr);
        }
    }

    // temp
    std::vector<VkPushConstantRange> defaultPushConstantRanges = {MeshPushConstants::range()};

    void createMaterialPipelines() {
        VkPipelineRenderingCreateInfoKHR pipelineRenderingCI = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainFormat,
            .depthAttachmentFormat = depthImage.imageFormat,
            .stencilAttachmentFormat = {}
        };

        GraphicsPipeline defaultPipeline(
            device, 
            &pipelineRenderingCI, 
            std::string("Shaders/triangle_mesh.vert.spv"), 
            std::string("Shaders/triangle_mesh.frag.spv"), 
            defaultPushConstantRanges,
            std::span<const VkDescriptorSetLayout>(sceneDescriptorSetLayouts.data(), 1), // TODO: They're all the same and this only has 1 layout for now
            {WINDOW_WIDTH, WINDOW_HEIGHT}
        );
        create_material(defaultPipeline, "defaultMaterial");
    }

    void init_scene_meshes() {

        // Sponza mesh
        Mesh sponzaMesh;
        load_mesh_from_gltf(sponzaMesh, ROOT_DIR "/Assets/Meshes/sponza-gltf/Sponza.gltf", false);
        Scene::GetInstance().sceneMeshMap["sponza"] = upload_mesh(sponzaMesh, vmaAllocator, mainDeletionQueue);

        RenderObject sponzaObject("defaultMaterial", "sponza");
        glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, -5.0f, 0.0f));
        glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.05f, 0.05f, 0.05f));
        sponzaObject.transformMatrix = translate * scale;

        Scene::GetInstance().sceneRenderObjects.push_back(sponzaObject);


        // Suzanne mesh
        Mesh monkeyMesh;
        load_mesh_from_gltf(monkeyMesh, ROOT_DIR "/Assets/Meshes/suzanne.glb", true);
        Scene::GetInstance().sceneMeshMap["suzanne"] = upload_mesh(monkeyMesh, vmaAllocator, mainDeletionQueue);

        RenderObject monkeyObject("defaultMaterial", "suzanne");
        glm::mat4 monkeyTranslate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 0.0f, 0.0f));
        monkeyObject.transformMatrix = monkeyTranslate;

        Scene::GetInstance().sceneRenderObjects.push_back(monkeyObject);

        // Helmet mesh
        Mesh helmetMesh;
        load_mesh_from_gltf(helmetMesh, ROOT_DIR "/Assets/Meshes/DamagedHelmet.glb", true);
        Scene::GetInstance().sceneMeshMap["helmet"] = upload_mesh(helmetMesh, vmaAllocator, mainDeletionQueue);

        RenderObject helmetObject("defaultMaterial", "helmet");
        glm::mat4 helmetTransform = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, 3.0f, 0.0f));
        helmetTransform = glm::rotate(helmetTransform, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
        helmetObject.transformMatrix = helmetTransform;

        Scene::GetInstance().sceneRenderObjects.push_back(helmetObject);
    }

    void init_imgui() {
        // Create a descriptor pool for IMGUI
        // The size of the pool is very oversized, but it's copied from imgui demo
        VkDescriptorPoolSize poolSizes[] = { 
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1000,
            .poolSizeCount = static_cast<uint32_t>(std::size(poolSizes)),
            .pPoolSizes = poolSizes
        };

        VkDescriptorPool imguiPool;
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiPool);

        // Initialize core structures of imgui library
        ImGui::CreateContext();

        // Initialize imgui for SDL
        ImGui_ImplSDL3_InitForVulkan(window);

        // Initialize imgui for Vulkan
        ImGui_ImplVulkan_InitInfo initInfo = {
            .Instance = instance,
            .PhysicalDevice = physicalDevice,
            .Device = device,
            .Queue = graphicsQueue,
            .DescriptorPool = imguiPool,
            .MinImageCount = 3,
            .ImageCount = 3,
            .UseDynamicRendering = true,
            .ColorAttachmentFormat = swapChainFormat,
        };

        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);

        // Execute a gpu command to upload imgui font textures
        // Update: No longer requires passing a command buffer (builds own pool + buffer internally), also no longer require destroy DestroyFontUploadObjects()
        // immediate_submit([=]() { 
            ImGui_ImplVulkan_CreateFontsTexture(); 
        // });

        // add the destroy the imgui created structures
        mainDeletionQueue.push_function([=]() {
            vkDestroyDescriptorPool(device, imguiPool, nullptr);
        });
    }

    // Used for data uploads and other "instant operations" not synced with the swapchain
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) { // Lambda should take a command buffer and return nothing
        
        vkResetFences(device, 1, &immediateFence);
        vkResetCommandBuffer(immediateCommandBuffer, {});

        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = {}
        };
        vkBeginCommandBuffer(immediateCommandBuffer, &beginInfo);

        function(immediateCommandBuffer);

        vkEndCommandBuffer(immediateCommandBuffer);

        // Submit immediate workload
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &immediateCommandBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, immediateFence);

        vkWaitForFences(device, 1, &immediateFence, true, (std::numeric_limits<uint64_t>::max)());
    }

    void draw_objects() {
        glm::mat4 view = camera.get_view_matrix();
        glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 200.0f);
        projection[1][1] *= -1; // flips the model because Vulkan uses positive Y downwards
        glm::mat4 viewProjectionMatrix = projection * view;

        for (RenderObject renderObject :  Scene::GetInstance().sceneRenderObjects) {
            renderObject.BindAndDraw(commandBuffers_F[currentFrame], viewProjectionMatrix, std::span<const VkDescriptorSet>(sceneDescriptorSets_F.data() + currentFrame, 1));
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
        // createDrawImage();
        createDepthImageAndView();
        createSynchronizationStructures();
        createCommandPool();
        createCommandBuffers();
        retrieveQueues();
        init_scene_lights();
        init_scene_descriptors();
        createMaterialPipelines();
        init_scene_meshes();
        init_imgui();
    }

    void draw_imgui(VkImageView targetImageView) {
        //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        VkRenderingAttachmentInfoKHR colorAttachment = rendering_attachment_info(
            targetImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, nullptr
        );
        VkRenderingInfoKHR renderingInfo = rendering_info_fullscreen(1, &colorAttachment, nullptr);

        PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
        vkCmdBeginRenderingKHR(commandBuffers_F[currentFrame], &renderingInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers_F[currentFrame]);

        PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));
        vkCmdEndRenderingKHR(commandBuffers_F[currentFrame]);
    }

    void update_scene_descriptors(uint32_t frameInFlightIndex) {
        int lightCircleRadius = 5;
        float lightCircleSpeed = 0.02f;
        Scene::GetInstance().scenePointLights[0].worldSpacePosition = glm::vec3(
            lightCircleRadius * glm::cos(lightCircleSpeed * frameNumber),
            0.0,
            lightCircleRadius * glm::sin(lightCircleSpeed * frameNumber)
        );
        

        update_buffer(
            PointLightsBuffers_F[frameInFlightIndex], 
            Scene::GetInstance().scenePointLights.size() * sizeof(PointLight),
            Scene::GetInstance().scenePointLights.data(),
            vmaAllocator
        );

        // Update descriptor set(s)
        VkDescriptorBufferInfo pointLightBufferInfo = {
            .buffer = PointLightsBuffers_F[frameInFlightIndex].buffer,
            .offset = 0,
            .range = Scene::GetInstance().scenePointLights.size() * sizeof(PointLight) // VK_WHOLE_SIZE?
        };

        VkWriteDescriptorSet pointLightDescriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = sceneDescriptorSets_F[frameInFlightIndex],
            .dstBinding = 0,
            .dstArrayElement = {},
            .descriptorCount = static_cast<uint32_t>(Scene::GetInstance().scenePointLights.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &pointLightBufferInfo,
            .pTexelBufferView = nullptr // ???
        };
        vkUpdateDescriptorSets(device, 1, &pointLightDescriptorWrite, 0, nullptr);
        
    }

    void drawFrame() {
            
            // Wait for previous frame to finish rendering before allowing us to acquire another image
            VkResult res = vkWaitForFences(device, 1, &renderFences_F[currentFrame], true, (std::numeric_limits<uint64_t>::max)());
            vkResetFences(device, 1, &renderFences_F[currentFrame]);
            update_scene_descriptors(currentFrame); // Executes immediately

            uint32_t imageIndex;
            res = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores_F[currentFrame], VK_NULL_HANDLE, &imageIndex);
            if (!((res == VK_SUCCESS) || (res == VK_SUBOPTIMAL_KHR))) {
                MRCERR(string_VkResult(res));
                throw std::runtime_error("Failed to acquire image from Swap Chain!");
            }
            vkResetCommandBuffer(commandBuffers_F[currentFrame], {});

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(commandBuffers_F[currentFrame], &beginInfo);

            {
                // Transition swapchain to color attachment write
                VkImageMemoryBarrier imb = image_memory_barrier(
                    swapChainImages[imageIndex], 
                    {}, 
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                );
                vkCmdPipelineBarrier(
                    commandBuffers_F[currentFrame], 
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    {},
                    0, nullptr,
                    0, nullptr,
                    1, &imb
                );
            }

            VkRenderingAttachmentInfoKHR colorAttachmentInfo = rendering_attachment_info(
                swapChainImageViews[imageIndex],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                &DEFAULT_CLEAR_VALUE_COLOR
            );

            VkRenderingAttachmentInfoKHR depthAttachmentInfo  = rendering_attachment_info(
                depthImage.imageView,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                &DEFAULT_CLEAR_VALUE_DEPTH
            );

            VkRenderingInfoKHR renderingInfo = rendering_info_fullscreen(
                1, &colorAttachmentInfo, &depthAttachmentInfo
            );

            PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
            vkCmdBeginRenderingKHR(commandBuffers_F[currentFrame], &renderingInfo);

            vkCmdSetViewport(commandBuffers_F[currentFrame], 0, 1, &DEFAULT_VIEWPORT_FULLSCREEN);
            vkCmdSetScissor(commandBuffers_F[currentFrame], 0, 1, &DEFAULT_SCISSOR_FULLSCREEN);
            draw_objects();

            PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));
            vkCmdEndRenderingKHR(commandBuffers_F[currentFrame]);



            // Draw imgui
            draw_imgui(swapChainImageViews[imageIndex]);

            {
                // Transition swapchain to correct presentation layout
                VkImageMemoryBarrier imb = image_memory_barrier(
                    swapChainImages[imageIndex], 
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    {},
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                );
                vkCmdPipelineBarrier(
                    commandBuffers_F[currentFrame], 
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    {},
                    0, nullptr,
                    0, nullptr,
                    1,&imb
                );
            }

            vkEndCommandBuffer(commandBuffers_F[currentFrame]);


            // Submit graphics workload
            VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &imageAvailableSemaphores_F[currentFrame];
            submitInfo.pWaitDstStageMask = &waitStageMask;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers_F[currentFrame];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphores_F[currentFrame];
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFences_F[currentFrame]);


            // Present frame
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &renderFinishedSemaphores_F[currentFrame];
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain;
            presentInfo.pImageIndices = &imageIndex;
            // res = vkQueuePresentKHR(presentQueue, &presentInfo);
            res = vkQueuePresentKHR(graphicsQueue, &presentInfo);


            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            frameNumber++;
            
    }

    void mainLoop() {
        SDL_Event sdlEvent;
        bool bQuit = false;
        ImGuiIO& io = ImGui::GetIO();
        UNUSED(io);
        while (!bQuit) {
            // Handle events on queue
            while (SDL_PollEvent(&sdlEvent) != 0) {
                ImGui_ImplSDL3_ProcessEvent(&sdlEvent);      
                if (sdlEvent.type == SDL_EVENT_QUIT) { // Built in Alt+F4 or hitting the 'x' button
                    SDL_SetRelativeMouseMode(SDL_FALSE); // Needed or else mouse freeze persists until clicking after closing app
                    bQuit = true;
                }
                // For single key presses
                if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
                    if (sdlEvent.key.keysym.sym == SDLK_TAB) {
                        interactableUI = !interactableUI;
                        if (!interactableUI) {
                            SDL_SetRelativeMouseMode(SDL_FALSE);
                            camera.freezeCamera();
                        } else {
                            SDL_SetRelativeMouseMode(SDL_TRUE);
                            camera.unfreezeCamera();
                        }
                    }
                }
                
                // Mouse motion
                if (sdlEvent.type == SDL_EVENT_MOUSE_MOTION) {
                    float xoffset = sdlEvent.motion.xrel;
                    float yoffset =  -sdlEvent.motion.yrel;
                    const float sensitivity = 0.1f;
                    xoffset *= sensitivity;
                    yoffset *= sensitivity;
                    camera.process_mouse_movement(xoffset, yoffset, true);
                }
            }
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            ImGui::ShowDemoWindow();
            ImGui::Render();
            drawFrame();

            lastFrameTick = currentFrameTick;
            currentFrameTick = SDL_GetTicks();
            deltaTime = (currentFrameTick - lastFrameTick) / 1000.f;

            // For multi-press/holding down keys
            const Uint8* state = SDL_GetKeyboardState(nullptr);
            if (state[SDL_SCANCODE_LSHIFT]) {
                cameraSpeed = 20.0f;
            } else {
                cameraSpeed = 10.0f;
            }
            if (state[SDL_SCANCODE_ESCAPE]) {
                SDL_SetRelativeMouseMode(SDL_FALSE); // Needed or else mouse freeze persists until clicking after closing app
                bQuit = true;
            }
            if (state[SDL_SCANCODE_W]) {
                camera.process_keyboard_input(CameraMovementDirection::FORWARD, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_S]) {
                camera.process_keyboard_input(CameraMovementDirection::BACKWARD, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_A]) {
                camera.process_keyboard_input(CameraMovementDirection::LEFT, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_D]) {
                camera.process_keyboard_input(CameraMovementDirection::RIGHT, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_SPACE]) {
                camera.process_keyboard_input(CameraMovementDirection::UP, cameraSpeed * deltaTime);
            }
            if (state[SDL_SCANCODE_LCTRL]) {
                camera.process_keyboard_input(CameraMovementDirection::DOWN, cameraSpeed * deltaTime);
            }
        }
    }

    void cleanup() {
        vkDeviceWaitIdle(device);

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        globalDescriptorAllocator.destroy_pool(device);

        for (auto material : Scene::GetInstance().sceneMaterialMap) {
            vkDestroyPipeline(device, material.second.getPipeline(), nullptr);
            vkDestroyPipelineLayout(device, material.second.getPipelineLayout(), nullptr);
        }
        mainDeletionQueue.flush();

        SDL_DestroyWindow(window);
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
