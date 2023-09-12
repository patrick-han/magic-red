#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <shaderc/shaderc.hpp>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <vulkan/vk_enum_string_helper.h>

#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>
#include <stdexcept>
#include <cstdlib>

#include "RootDir.h"

#include "Callbacks.h"
#include "Utils.h"
#include "Shader.h"
#include "Types.h"
#include "Mesh.h"
#include "Buffer.h"
#include "Image.h"
#include "Material.h"
#include "RenderObject.h"


constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr bool bDepthTest = true;
constexpr bool bDepthWrite = true;

int frameNumber = 0;

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 renderMatrix;
};

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
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    // Images
    vk::Format depthFormat = vk::Format::eD32Sfloat;
    AllocatedImage depthImage;
    vk::ImageView depthImageView;

    // Swapchain
    uint32_t swapChainImageCount;
    vk::Extent2D swapChainExtent;
    vk::UniqueSwapchainKHR swapChain;
    vk::Format swapChainFormat;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::UniqueImageView> swapChainImageViews;

    // Shaders
    vk::UniqueShaderModule vertexShaderModule;
    vk::UniqueShaderModule fragmentShaderModule;

    // Synchronization (per in-flight frame resources)
    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
    std::vector<vk::UniqueFence> renderFences;
    uint32_t currentFrame = 0;

    // Renderpass
    vk::UniqueRenderPass renderPass;

    // Graphics Pipeline
    vk::UniquePipelineLayout meshPipelineLayout;
    vk::UniquePipeline meshPipeline;

    // Framebuffers
    std::vector<vk::UniqueFramebuffer> framebuffers;

    // Command Pools and Buffers
    vk::UniqueCommandPool commandPoolUnique;
    std::vector<vk::UniqueCommandBuffer> commandBuffers; // (per in-flight frame resource)

    // Resources
    DeletionQueue mainDeletionQueue;

    // Scene
    std::vector<RenderObject> sceneRenderObjects;
    std::unordered_map<std::string, Material> sceneMaterialMap;
    std::unordered_map<std::string, Mesh> sceneMeshMap;

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
        vk::InstanceCreateFlagBits instanceCreateFlagBits = {};
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
            throw std::runtime_error("Could not create surface!");
        }
        surface = vk::UniqueSurfaceKHR(surfaceTmp, *instance);
    }

    void initPhysicalDevice() {
        std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();
        for (vk::PhysicalDevice& d : physicalDevices) {
            MRLOG("Physical device enumerated: " << d.getProperties().deviceName);
        }

        // Find Apple device if Apple build, otherwise just choose the first device in the list
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
            physicalDevice = physicalDevices[0]; // TODO: By Default, just select the first physical device, could be bad if there are both integrated and discrete GPUs in the system
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
        
        // Query for surface format support
        vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
        std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(*surface);
        swapChainFormat = vk::Format::eB8G8R8A8Unorm;
        vk::ColorSpaceKHR swapChainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
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
        swapChainExtent = vk::Extent2D{ WINDOW_WIDTH, WINDOW_HEIGHT };
        swapChainImageCount = 2; // Should probably request support for this, but it's probably fine
        vk::SwapchainCreateInfoKHR swapChainCreateInfo = {
            {}, 
            surface.get(), 
            swapChainImageCount, 
            swapChainFormat,
            vk::ColorSpaceKHR::eSrgbNonlinear, 
            swapChainExtent, 
            1, 
            vk::ImageUsageFlagBits::eColorAttachment,
            sharingModeUtil.sharingMode, 
            sharingModeUtil.familyIndicesCount,
            sharingModeUtil.familyIndicesDataPtr, 
            vk::SurfaceTransformFlagBitsKHR::eIdentity,
            vk::CompositeAlphaFlagBitsKHR::eOpaque, 
            vk::PresentModeKHR::eFifo, // Required for implementations, no need to query for support
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

    void createDepthImageAndView() {
        vk::Extent3D depthExtent = vk::Extent3D{ swapChainExtent, 1 };

        vk::ImageCreateInfo depthImageCreateInfo = {{}, vk::ImageType::e2D, depthFormat, depthExtent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined};

        VkImageCreateInfo ci = static_cast<VkImageCreateInfo>(depthImageCreateInfo);

        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaAllocInfo.requiredFlags = static_cast<VkMemoryPropertyFlagBits>(vk::MemoryPropertyFlagBits::eDeviceLocal);
        VkResult res = vmaCreateImage(vmaAllocator, &ci, &vmaAllocInfo, &reinterpret_cast<VkImage &>(depthImage.image), &depthImage.allocation, nullptr);

        if (res != VK_SUCCESS) {
            MRCERR(string_VkResult(res));
            throw std::runtime_error("Could not create depth image!");
        }

        vk::ImageSubresourceRange depthSubresRange = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1};
        vk::ImageViewCreateInfo depthImageViewCreateInfo = {{}, depthImage.image, vk::ImageViewType::e2D, depthFormat, {}, depthSubresRange};
        depthImageView = device.get().createImageView(depthImageViewCreateInfo);

        mainDeletionQueue.push_function([=]() {
            device.get().destroyImageView(depthImageView, nullptr);
            vmaDestroyImage(vmaAllocator, depthImage.image, depthImage.allocation);
        });
    }

    void createShaderModules() {
        // TODO: Hardcoded for now
        std::string vertexShaderSource = load_shader_source_to_string(ROOT_DIR "shaders/triangle_mesh.vert");
        std::string fragmentShaderSource = load_shader_source_to_string(ROOT_DIR "shaders/triangle_mesh.frag");

        vertexShaderModule = compile_shader(device.get(), vertexShaderSource, shaderc_glsl_vertex_shader, "vertex shader");
        fragmentShaderModule = compile_shader(device.get(), fragmentShaderSource, shaderc_glsl_fragment_shader, "fragment shader");

        vk::Device d = device.get();
        VkDevice d2 = reinterpret_cast<VkDevice&>(d);
    }

    void createSynchronizationStructures() {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
            imageAvailableSemaphores.push_back(device->createSemaphoreUnique(semaphoreCreateInfo));
            renderFinishedSemaphores.push_back(device->createSemaphoreUnique(semaphoreCreateInfo));

            vk::FenceCreateInfo fenceCreateInfo = {vk::FenceCreateFlagBits::eSignaled};
            renderFences.push_back(device->createFenceUnique(fenceCreateInfo));
        }
        

    }

    void createRenderPass() {
        // Prepare attachment descriptions and references
        vk::AttachmentDescription colorAttachment = { {}, swapChainFormat, vk::SampleCountFlagBits::e1, 
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            {}, 
            {}, 
            {}, 
            vk::ImageLayout::ePresentSrcKHR
        };
        vk::AttachmentReference colorAttachmentRef = { 0, vk::ImageLayout::eColorAttachmentOptimal };

        vk::AttachmentDescription depthAttachment = { {}, depthFormat, vk::SampleCountFlagBits::e1, 
            vk::AttachmentLoadOp::eClear,     // Color/depth
            vk::AttachmentStoreOp::eStore,    // Color/depth
            vk::AttachmentLoadOp::eClear,     // Stencil
            vk::AttachmentStoreOp::eDontCare, // Stencil
            vk::ImageLayout::eUndefined,                    // Renderpass instance begin layout
            vk::ImageLayout::eDepthStencilAttachmentOptimal // Renderpass instance end layout
        };
        vk::AttachmentReference depthAttachmentRef = { 1, vk::ImageLayout::eDepthStencilAttachmentOptimal }; // Layout during the subpass

        vk::SubpassDescription subpass = { {}, 
            vk::PipelineBindPoint::eGraphics, 
            0,                  // Input attachment count
            nullptr, 
            1, 
            &colorAttachmentRef, 
            {},                 // Resolve attachments
            &depthAttachmentRef, 
            {}, {}
        };

        // Color attachment synchronization
        vk::SubpassDependency subpassDependencyColor = { // TODO: Understand
            VK_SUBPASS_EXTERNAL,                               // srcSubpass
            0,                                                 // dstSubpass
            vk::PipelineStageFlagBits::eColorAttachmentOutput, // srcStageMask This should wait for swapchain to finish reading from the image
            vk::PipelineStageFlagBits::eColorAttachmentOutput, // dstStageMask
            {},                                                // srcAccessMask
            vk::AccessFlagBits::eColorAttachmentWrite,         // dstAccessMask
            {} // dependencyFlags
        };

        // Depth attachment synchronization
        vk::SubpassDependency subpassDependencyDepth = { // TODO: Understand
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests, // Potentially used in either
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
            {},
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            {} // dependencyFlags
        };

        vk::AttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
        vk::SubpassDependency subpassDependencies[2] = { subpassDependencyColor, subpassDependencyDepth };

        renderPass = device->createRenderPassUnique(
            vk::RenderPassCreateInfo{ {}, 
                2, 
                attachments, 
                1, 
                &subpass, 
                2, 
                subpassDependencies
            }
        );
    }

    void createGraphicsPipeline() {
        vk::PipelineShaderStageCreateInfo vertShaderStageInfo = { {}, vk::ShaderStageFlagBits::eVertex, *vertexShaderModule, "main"};
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo = { {}, vk::ShaderStageFlagBits::eFragment, *fragmentShaderModule, "main"};
        std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = { vertShaderStageInfo, fragShaderStageInfo };
        
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = { {}, 0u, nullptr, 0u, nullptr }; // TODO: Hardcoded shader for now
        VertexInputDescription vertexDescription = get_vertex_description();
        vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();
        vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
        vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
        
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly = { {}, vk::PrimitiveTopology::eTriangleList, false };
        vk::Viewport viewport = { 0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT), 0.0f, 1.0f };
        vk::Rect2D scissor = { { 0, 0 }, swapChainExtent };
        vk::PipelineViewportStateCreateInfo viewportState = { {}, 1, &viewport, 1, &scissor };
        vk::PipelineRasterizationStateCreateInfo rasterizer = { {}, /*depthClamp*/ false,
        /*rasterizeDiscard*/ false, vk::PolygonMode::eFill, {},
        /*frontFace*/ vk::FrontFace::eCounterClockwise, {}, {}, {}, {}, 1.0f };

        vk::PipelineDepthStencilStateCreateInfo depthStencil = { {}, 
            bDepthTest ? VK_TRUE : VK_FALSE,
            bDepthWrite ? VK_TRUE : VK_FALSE,
            vk::CompareOp::eLessOrEqual,
            VK_FALSE, // depth bounds test
            VK_FALSE, // stencil
            {}, {}, {}, {}
        };

        vk::PipelineMultisampleStateCreateInfo multisampling = { {}, vk::SampleCountFlagBits::e1, false, 1.0 };

        vk::PipelineColorBlendAttachmentState colorBlendAttachment = { {}, /*srcCol*/ vk::BlendFactor::eOne,
        /*dstCol*/ vk::BlendFactor::eZero, /*colBlend*/ vk::BlendOp::eAdd,
        /*srcAlpha*/ vk::BlendFactor::eOne, /*dstAlpha*/ vk::BlendFactor::eZero,
        /*alphaBlend*/ vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
        vk::PipelineColorBlendStateCreateInfo colorBlending = { {}, /*logicOpEnable=*/false, vk::LogicOp::eCopy, /*attachmentCount=*/1, /*colourAttachments=*/&colorBlendAttachment };

        std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Push constants
        vk::PushConstantRange meshPushConstant = {};
        meshPushConstant.offset = 0;
        meshPushConstant.size = sizeof(MeshPushConstants);
        meshPushConstant.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::PipelineLayoutCreateInfo meshPipelineLayoutCreateInfo = {};
        meshPipelineLayoutCreateInfo.flags = {};
        meshPipelineLayoutCreateInfo.setLayoutCount = 0;
        meshPipelineLayoutCreateInfo.pSetLayouts = nullptr;
        meshPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        meshPipelineLayoutCreateInfo.pPushConstantRanges = &meshPushConstant;

        meshPipelineLayout = device->createPipelineLayoutUnique(meshPipelineLayoutCreateInfo, nullptr);

        vk::GraphicsPipelineCreateInfo pipelineCreateInfo = {
            {}, 
            2, 
            pipelineShaderStages.data(), 
            &vertexInputInfo, 
            &inputAssembly, 
            nullptr, 
            &viewportState, 
            &rasterizer, 
            &multisampling,
            &depthStencil,
            &colorBlending,
            &dynamicState, // Dynamic State
            *meshPipelineLayout,
            *renderPass,
            0
        };

        meshPipeline = device->createGraphicsPipelineUnique({}, pipelineCreateInfo).value;

        create_material(meshPipeline.get(), meshPipelineLayout.get(), "defaultMesh", sceneMaterialMap);
    }

    void createFramebuffers() {
        framebuffers = std::vector<vk::UniqueFramebuffer>(swapChainImageCount);

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vk::ImageView attachments[2] = { swapChainImageViews[i].get(), depthImageView};

            framebuffers[i] = device->createFramebufferUnique(vk::FramebufferCreateInfo{
            {},
            *renderPass,
            2,
            attachments,
            swapChainExtent.width,
            swapChainExtent.height,
            1 });
        }
    }

    void createCommandPool() {
        commandPoolUnique = device->createCommandPoolUnique({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, static_cast<uint32_t>(graphicsQueueFamilyIndex) });
    }
    

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        commandBuffers = device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
            commandPoolUnique.get(),
            vk::CommandBufferLevel::ePrimary,
            static_cast<uint32_t>(commandBuffers.size())));
    }

    void retrieveQueues() {
        graphicsQueue = device->getQueue(static_cast<uint32_t>(graphicsQueueFamilyIndex), 0);
        presentQueue = device->getQueue(static_cast<uint32_t>(presentQueueFamilyIndex), 0);
    }

    void initVMA() {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device.get();
        allocatorInfo.instance = instance.get();
        VkResult res = vmaCreateAllocator(&allocatorInfo, &vmaAllocator);
        if (res != VK_SUCCESS) {
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

        upload_mesh(triangleMesh, vmaAllocator, mainDeletionQueue);
        sceneMeshMap["triangle"] = triangleMesh;
        
        // Suzanne mesh
        Mesh monkeyMesh;
        load_mesh_from_obj(monkeyMesh, ROOT_DIR "/assets/meshes/suzanne.obj");
        upload_mesh(monkeyMesh, vmaAllocator, mainDeletionQueue);
        sceneMeshMap["suzanne"] = monkeyMesh;

        // Sponza mesh
        Mesh sponzaMesh;
        load_mesh_from_obj(sponzaMesh, ROOT_DIR "/assets/meshes/sponza.obj");
        upload_mesh(sponzaMesh, vmaAllocator, mainDeletionQueue);
        sceneMeshMap["sponza"] = sponzaMesh;
    }

    void init_scene() {
        RenderObject sponzaObject;
        sponzaObject.material = get_material("defaultMesh", sceneMaterialMap);
        sponzaObject.mesh = get_mesh("sponza", sceneMeshMap);
        glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(0.0f, -5.0f, 0.0f));
        glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.1, 0.1, 0.1));
        sponzaObject.transformMatrix = translate * scale;

        sceneRenderObjects.push_back(sponzaObject);

        RenderObject monkeyObject;
        monkeyObject.material = get_material("defaultMesh", sceneMaterialMap);
        monkeyObject.mesh = get_mesh("suzanne", sceneMeshMap);
        monkeyObject.transformMatrix = glm::mat4{ 1.0f };

        sceneRenderObjects.push_back(monkeyObject);

        for (int x = -10; x < 10; x++) {
            for (int y = -10; y < 10; y++) {
                RenderObject triangleObject;
                triangleObject.material = get_material("defaultMesh", sceneMaterialMap);
                triangleObject.mesh = get_mesh("triangle", sceneMeshMap);
                glm::mat4 translate = glm::translate(glm::mat4{ 1.0f }, glm::vec3(x, 0.0f, y));
                glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
                triangleObject.transformMatrix = translate * scale;
                
                sceneRenderObjects.push_back(triangleObject);
            }
        }
    }

    void draw_objects(uint32_t imageIndex) {
        

        glm::vec3 camPos = { 0.f,-2.f,-8.f };
        glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
        glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 200.0f);
        projection[1][1] *= -1; // flips the model

        // Bind the first pipeline
        vk::Pipeline previousPipeline = sceneRenderObjects[0].material->pipeline;
        commandBuffers[currentFrame]->bindPipeline(vk::PipelineBindPoint::eGraphics, previousPipeline);

        for (RenderObject renderObject : sceneRenderObjects) {
            
            if (previousPipeline != renderObject.material->pipeline) {
                commandBuffers[currentFrame]->bindPipeline(vk::PipelineBindPoint::eGraphics, renderObject.material->pipeline);
                previousPipeline = renderObject.material->pipeline;
            }

            vk::DeviceSize offset = 0;
            
            commandBuffers[currentFrame]->bindVertexBuffers(0, 1, &renderObject.mesh->vertexBuffer.buffer, &offset);

            vk::Viewport viewport = { 0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f };
            commandBuffers[currentFrame]->setViewport(0, 1, &viewport);

            vk::Rect2D scissor = { {0, 0}, swapChainExtent};
            commandBuffers[currentFrame]->setScissor(0, 1, &scissor);

            // Compute MVP matrix
            glm::mat4 model = renderObject.transformMatrix;
            glm::mat4 rotate = glm::rotate(glm::mat4{ 1.0f }, glm::radians(frameNumber * 0.4f), glm::vec3(0, 1, 0));
            model = rotate * model;
            glm::mat4 mvpMatrix = projection * view * model;

            MeshPushConstants constants;
            constants.renderMatrix = mvpMatrix;
            commandBuffers[currentFrame]->pushConstants(meshPipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(MeshPushConstants), &constants);
            commandBuffers[currentFrame]->draw(renderObject.mesh->vertices.size(), 1, 0, 0);
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
        createShaderModules();
        createSynchronizationStructures();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        retrieveQueues();
        load_meshes();
        init_scene();

    }

    void drawFrame() {
            // Wait for previous frame to finish rendering before allowing us to acquire another image
            vk::Result res = device->waitForFences(renderFences[currentFrame].get(), true, (std::numeric_limits<uint64_t>::max)());
            device->resetFences(renderFences[currentFrame].get());

            // Retrieve image index to list of swapchain-backed framebuffers and re-record command buffers
            vk::ResultValue<uint32_t> imageIndex = device->acquireNextImageKHR(swapChain.get(), std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame].get(), {});
            commandBuffers[currentFrame]->reset();

            vk::ClearValue clearValue = {{0.0f, 0.0f, 0.5f, 1.0f}};
            vk::ClearValue clearValueDepth = {{1.0f, 0}};
            vk::ClearValue clearValues[2] = { clearValue, clearValueDepth };

            vk::CommandBufferBeginInfo beginInfo = {};
            commandBuffers[currentFrame]->begin(beginInfo);
            vk::RenderPassBeginInfo renderPassBeginInfo = { renderPass.get(), framebuffers[imageIndex.value].get(), vk::Rect2D{ { 0, 0 }, swapChainExtent }, 
                2,
                clearValues 
            };

            commandBuffers[currentFrame]->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
            draw_objects(imageIndex.value);
            commandBuffers[currentFrame]->endRenderPass();
            commandBuffers[currentFrame]->end();

            // Submit graphics workload
            vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            vk::SubmitInfo submitInfo = { 1, &imageAvailableSemaphores[currentFrame].get(), &waitStageMask, 1, &commandBuffers[currentFrame].get(), 1, &renderFinishedSemaphores[currentFrame].get() };
            graphicsQueue.submit(submitInfo, renderFences[currentFrame].get());

            // Present frame
            vk::PresentInfoKHR presentInfo = { 1, &renderFinishedSemaphores[currentFrame].get(), 1, &swapChain.get(), &imageIndex.value };
            vk::Result result = presentQueue.presentKHR(presentInfo);

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            frameNumber++;
            
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            drawFrame();
        }
    }

    void cleanup() {
        device->waitIdle();

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