#include "GfxDevice.h"
#include <Common/Platform.h>
#include <SDL3/SDL_vulkan.h>
#include <Common/Debug.h>
#include <Common/Config.h>
#include <vulkan/vk_enum_string_helper.h> // Doesn't work on linux?
#include <cassert>

void GfxDevice::create_instance() {
    // Specify application and engine info
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "Magic Red", VK_MAKE_API_VERSION(1, 0, 0, 0), "Magic Red", VK_MAKE_API_VERSION(1, 0, 0, 0), VK_API_VERSION_1_2};

    // Get extensions required for SDL VK surface rendering
    uint32_t sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
    m_extensionsVector.resize(sdlExtensionCount);
    const char* const* sdlVulkanExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
    for (uint32_t i = 0; i < sdlExtensionCount; i++) {
        m_extensionsVector[i] = sdlVulkanExtensions[i];
    }
    

    // MoltenVK requires
    VkInstanceCreateFlags instanceCreateFlagBits = {};
#if PLATFORM_MACOS
    MRLOG("Running on an Apple device, adding appropriate extension and instance creation flag bits");
    m_extensionsVector.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    instanceCreateFlagBits |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    // Enable validation layers and creation of debug messenger if building debug
    if (bEnableValidationLayers) {
        MRLOG("Debug build");
        m_extensionsVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        m_layers.push_back("VK_LAYER_KHRONOS_validation");
    } else {
        MRLOG("Release build");
    }

    // Create instance
    VkInstanceCreateInfo instanceCreateInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        instanceCreateFlagBits,
        &appInfo,
        static_cast<uint32_t>(m_layers.size()),
        m_layers.data(),
        static_cast<uint32_t>(m_extensionsVector.size()),
        m_extensionsVector.data()
    };
    VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
    if (res != VK_SUCCESS) {
        MRCERR(string_VkResult(res));
        MRCERR("Failed to create instance!");
    }
    m_mainDeletionQueue.push_function([=]() {
        vkDestroyInstance(m_instance, nullptr);
    });
}

void GfxDevice::create_debug_messenger() {
    if (!bEnableValidationLayers) {
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

    VkResult res = CreateDebugUtilsMessengerEXT(m_instance, &debugMessengerCreateInfo, nullptr, &m_debugMessenger);
    if (res != VK_SUCCESS) {
        MRCERR(string_VkResult(res));
        MRCERR("Failed to create debug messenger!");
    }
    m_mainDeletionQueue.push_function([=]() {
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    });
}

void GfxDevice::create_surface(SDL_Window * const window) {
    SDL_bool res = SDL_Vulkan_CreateSurface(window, m_instance, nullptr, &m_surface);
    if (res != SDL_TRUE) {
        MRCERR("Could not create surface!");
    }
    m_mainDeletionQueue.push_function([=]() {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    });
}

void GfxDevice::init_physical_device() {
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
    if (physicalDeviceCount == 0) {
        MRCERR("No Vulkan capable devices found");
    }
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

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
    m_physicalDevice = physicalDevices[std::distance(physicalDevices.begin(), findIfIterator)];
    VkPhysicalDeviceProperties chosenPhysDeviceProps;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &chosenPhysDeviceProps);
    MRLOG("Chosen deviceName: " << chosenPhysDeviceProps.deviceName);
#else
    m_physicalDevice = physicalDevices[0]; // TODO: By Default, just select the first physical device, could be bad if there are both integrated and discrete GPUs in the system
#endif
}

void GfxDevice::find_queue_family_indices() {
    // Find graphics and present queue family indices
    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    m_graphicsQueueFamilyIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(),
        std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
            [](VkQueueFamilyProperties const& qfp) {
                return qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            }
        )
    ));
    m_presentQueueFamilyIndex = 0u;
    for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++) {
        // Check if a given queue family on our device supports presentation to the surface that was created
        VkBool32 supported = VK_FALSE;
        VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, static_cast<uint32_t>(queueFamilyIndex), m_surface, &supported);
        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            MRCERR("Queue family does not support presentation!");
        } else {
            m_presentQueueFamilyIndex = queueFamilyIndex;
        }
    }
    MRLOG("Graphics and Present queue family indices, respectively: " << m_graphicsQueueFamilyIndex << ", " << m_presentQueueFamilyIndex);

    m_uniqueQueueFamilyIndices = {
        m_graphicsQueueFamilyIndex,
        m_presentQueueFamilyIndex
    };

    m_FamilyIndices = {
        m_uniqueQueueFamilyIndices.begin(),
        m_uniqueQueueFamilyIndices.end()
    };
}

void GfxDevice::create_device() {
    // Creation of logical device requires queue creation info as well as extensions + layers we want

    // For each queue family, create a single queue
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 0.0f;
    uint32_t queueCountPerFamily = 1;
    for (auto& queueFamilyIndex : m_uniqueQueueFamilyIndices) {
        queueCreateInfos.push_back(VkDeviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, VkDeviceQueueCreateFlags(), static_cast<uint32_t>(queueFamilyIndex), queueCountPerFamily, &queuePriority });
    }

    // TODO:
    // Actually check if things are supported

    // Device extensions
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_dynamic_rendering", "VK_KHR_buffer_device_address", "VK_EXT_scalar_block_layout", "VK_EXT_descriptor_indexing"};
#if PLATFORM_MACOS
    deviceExtensions.push_back("VK_KHR_portability_subset");
#endif
    
    // Needed to enable dynamic rendering extension
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_feature {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR,
        .pNext = &dynamic_rendering_feature,
        .bufferDeviceAddress = VK_TRUE,
#ifdef NDEBUG
        .bufferDeviceAddressCaptureReplay = VK_TRUE,
#endif
    };

    VkPhysicalDeviceScalarBlockLayoutFeatures scalar_block_layout_feature {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT,
        .pNext = &buffer_device_address_feature,
        .scalarBlockLayout = VK_TRUE
    };

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_feature {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
        .pNext = &scalar_block_layout_feature,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE, // Indicates whether the implementation supports statically using a descriptor set binding in which some descriptors are not valid
        .runtimeDescriptorArray = VK_TRUE // Indicates whether the implementation supports the SPIR-V RuntimeDescriptorArray capability. 
   
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        &descriptor_indexing_feature,
        VkDeviceCreateFlags(),
        static_cast<uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data(),
        0u,
        nullptr,
        static_cast<uint32_t>(deviceExtensions.size()),
        deviceExtensions.data(),
        nullptr
    };
    vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    m_mainDeletionQueue.push_function([=]() {
        vkDestroyDevice(m_device, nullptr);
    });
}


void GfxDevice::create_swap_chain() {
    // If the graphics and presentation queue family indices are different, allow concurrent access to object from multiple queue families
    struct SM {
        VkSharingMode sharingMode;
        uint32_t familyIndicesCount;
        uint32_t* familyIndicesDataPtr;
    } sharingModeUtil  = { (m_graphicsQueueFamilyIndex != m_presentQueueFamilyIndex) ?
                        SM{ VK_SHARING_MODE_CONCURRENT, 2u, m_FamilyIndices.data() } :
                        SM{ VK_SHARING_MODE_EXCLUSIVE, 0u, static_cast<uint32_t*>(nullptr) } };
    
    // Query for surface format support
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &surfaceFormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &surfaceFormatCount, formats.data());
    
    m_swapChainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR swapChainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    bool foundCompatible = false;
    for (auto format : formats) {
        if (format.format == m_swapChainFormat && format.colorSpace == swapChainColorSpace) {
            foundCompatible = true;
            break;
        }
    }
    if (!foundCompatible) {
        MRCERR("Could not find compatible surface format!");
    }

    // Create swapchain
    VkExtent2D swapChainExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        {},
        m_surface, 
        m_swapChainImageCount, // This is a minimum, not exact
        m_swapChainFormat,
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
    VkResult res = vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapChain);
    if (res != VK_SUCCESS) {
        MRCERR(string_VkResult(res));
        MRCERR("Could not create swap chain!");
    }
    m_mainDeletionQueue.push_function([=]() {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    });
}

 void GfxDevice::get_swap_chain_images() {
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_swapChainImageCount, nullptr); // We only specified a minimum during creation, need to query for the real number
    m_swapChainImages.resize(m_swapChainImageCount); 
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_swapChainImageCount, m_swapChainImages.data());
    MRLOG("Final number of swapchain images: " << m_swapChainImageCount);
    assert(m_swapChainImageCount >= MAX_FRAMES_IN_FLIGHT); // Need at least as many swapchain images as FiFs or the extra FiFs are useless
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (uint32_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkImageViewCreateInfo imageViewCreateInfo = imageview_create_info(m_swapChainImages[i], m_swapChainFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
        VkResult res = vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_swapChainImageViews[i]);
        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            MRCERR("Could not create swap chain image view!");
        }
    }
    m_mainDeletionQueue.push_function([=]() {
        for (uint32_t k = 0; k < m_swapChainImageViews.size(); k++) {
            vkDestroyImageView(m_device, m_swapChainImageViews[k], nullptr);
        }   
    });
}

// void GfxDevice::create_draw_image() {
//     VkExtent3D drawImageExtent = {
//         WINDOW_WIDTH,
//         WINDOW_HEIGHT,
//         1
//     };
//     m_drawImage.imageExtent = drawImageExtent;
//     m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    
//     VkImageUsageFlags drawImageUsages = {};
//     drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
//     drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//     drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
//     drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

//     VkImageCreateInfo drawImageCreateInfo = image_create_info(m_drawImage.imageFormat, m_drawImage.imageExtent, drawImageUsages);

//     upload_gpu_only_image(m_drawImage, drawImageCreateInfo, m_vmaAllocator, m_mainDeletionQueue);

//     VkImageViewCreateInfo drawImageViewCreateInfo = imageview_create_info(m_drawImage.image, m_drawImage.imageFormat, {}, VK_IMAGE_ASPECT_COLOR_BIT);
//     VkResult res = vkCreateImageView(m_device, &drawImageViewCreateInfo, nullptr, &m_drawImage.imageView);
//     if(res != VK_SUCCESS) {
//         MRCERR(string_VkResult(res));
//         MRCERR("Could not create draw image view!");
//     }

//     m_mainDeletionQueue.push_function([=]() {
//         vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
//     });
// }

 void GfxDevice::create_depth_image_and_view() {
    m_depthImage.imageExtent = VkExtent3D{ WINDOW_WIDTH, WINDOW_HEIGHT, 1 };
    m_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo depthImageCreateInfo = image_create_info(m_depthImage.imageFormat, m_depthImage.imageExtent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TYPE_2D);

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; 
    VkResult res = vmaCreateImage(m_vmaAllocator, &depthImageCreateInfo, &vmaAllocInfo, &reinterpret_cast<VkImage &>(m_depthImage.image), &m_depthImage.allocation, nullptr);

    if (res != VK_SUCCESS) {
        MRCERR(string_VkResult(res));
        MRCERR("Could not create depth image!");
    }

    VkImageViewCreateInfo depthImageViewCreateInfo = imageview_create_info(m_depthImage.image, m_depthImage.imageFormat, {},VK_IMAGE_ASPECT_DEPTH_BIT);

    vkCreateImageView(m_device, &depthImageViewCreateInfo, nullptr, &m_depthImage.imageView);

    m_mainDeletionQueue.push_function([=]() {
        vkDestroyImageView(m_device, m_depthImage.imageView, nullptr);
        vmaDestroyImage(m_vmaAllocator, m_depthImage.image, m_depthImage.allocation);
    });
}

void GfxDevice::create_synchronization_structures() {
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, {}, {}};
        vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphores[i]);
        vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphores[i]);
        m_mainDeletionQueue.push_function([=]() {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        });

        VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
        vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_renderFences[i]);
        m_mainDeletionQueue.push_function([=]() {
            vkDestroyFence(m_device, m_renderFences[i], nullptr);
        });
    }

    // Immediate submission synchronization structures
    {
        VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
        vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immediateFence);
        m_mainDeletionQueue.push_function([=]() {
            vkDestroyFence(m_device, m_immediateFence, nullptr);
        });
    }
}

void GfxDevice::create_command_pool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer.
    m_graphicsQueueFamilyIndex};
    vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPool);
    vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_immediateCommandPool);

    m_mainDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        vkDestroyCommandPool(m_device, m_immediateCommandPool, nullptr);
    });
}


void GfxDevice::create_command_buffers() {
    // Main per FiF command buffers
        VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.commandPool = m_commandPool;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    vkAllocateCommandBuffers(m_device, &cmdBufferAllocInfo, m_commandBuffers.data());

    // Immediate command buffer
    VkCommandBufferAllocateInfo immCmdBufferAllocInfo = {};
    immCmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    immCmdBufferAllocInfo.commandPool = m_immediateCommandPool;
    immCmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    immCmdBufferAllocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(m_device, &immCmdBufferAllocInfo, &m_immediateCommandBuffer);
}

void GfxDevice::retrieve_queues() {
    vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentQueueFamilyIndex, 0, &m_presentQueue);
}

void GfxDevice::init_VMA() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    VkResult res = vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
    if (res != VK_SUCCESS) {
        MRCERR(string_VkResult(res));
        MRCERR("vmaCreateAllocator unsuccessful!");
    }
    m_mainDeletionQueue.push_function([&]() {
        vmaDestroyAllocator(m_vmaAllocator);
    });
}


void GfxDevice::init(SDL_Window * const window) {
    create_instance();
    create_debug_messenger();
    create_surface(window);
    init_physical_device();
    find_queue_family_indices();
    create_device();
    init_VMA();
    create_swap_chain();
    get_swap_chain_images();
    // create_draw_image();
    create_depth_image_and_view();
    create_synchronization_structures();
    create_command_pool();
    create_command_buffers();
    retrieve_queues();
}

// Used for data uploads and other "instant operations" not synced with the swapchain
// From vkguide.dev: https://vkguide.dev/docs/new_chapter_2/vulkan_imgui_setup/
void GfxDevice::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) const { // Lambda should take a command buffer and return nothing
    
    vkResetFences(m_device, 1, &m_immediateFence);
    vkResetCommandBuffer(m_immediateCommandBuffer, {});

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = {}
    };
    vkBeginCommandBuffer(m_immediateCommandBuffer, &beginInfo);

    function(m_immediateCommandBuffer);

    vkEndCommandBuffer(m_immediateCommandBuffer);

    // Submit immediate workload
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_immediateCommandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_immediateFence);

    vkWaitForFences(m_device, 1, &m_immediateFence, true, (std::numeric_limits<uint64_t>::max)());
}

VkInstance GfxDevice::get_instance() const { return m_instance; }

VkQueue GfxDevice::get_graphics_queue() const { return m_graphicsQueue; }

VkPhysicalDevice GfxDevice::get_physical_device() const { return m_physicalDevice; }

VkCommandBuffer GfxDevice::get_frame_command_buffer(uint32_t currentFrameIndex) const { return m_commandBuffers[currentFrameIndex]; };

VkSemaphore GfxDevice::get_frame_imageAvailableSemaphore(uint32_t currentFrameIndex) const { return m_imageAvailableSemaphores[currentFrameIndex]; };

VkSemaphore GfxDevice::get_frame_renderFinishedSemaphore(uint32_t currentFrameIndex) const { return m_renderFinishedSemaphores[currentFrameIndex];};

VkFence GfxDevice::get_frame_fence(uint32_t currentFrameIndex) const { return m_renderFences[currentFrameIndex]; };

void GfxDevice::cleanup() { m_mainDeletionQueue.flush(); }
