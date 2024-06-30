#pragma once
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <Wrappers/Image.h>
#include <Wrappers/Buffer.h>
#include <Descriptor/Descriptor.h>
#include <set>
#include <DeletionQueue.h>

#include <IncludeHelpers/VmaIncludes.h>

class GfxDevice
{
public:
    // VulkanMemoryAllocator (VMA)
    VmaAllocator m_vmaAllocator;
private:
    // Instance
    std::vector<const char*> m_extensionsVector;
    std::vector<const char*> m_layers;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;

    // Surface and devices
    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;

    // Queues
    uint32_t m_graphicsQueueFamilyIndex;
    uint32_t m_presentQueueFamilyIndex;
    std::set<uint32_t> m_uniqueQueueFamilyIndices;
    std::vector<uint32_t> m_FamilyIndices;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    // Images
//    AllocatedImage m_drawImage;
    // VkExtent2D m_drawExtent; // Allow for resizing

    // Swapchain
    
    uint32_t m_swapChainImageCount = 3; // Should probably request support for this, but it's probably fine

    // Synchronization
    std::vector<VkSemaphore> m_imageAvailableSemaphores_F;
    std::vector<VkSemaphore> m_renderFinishedSemaphores_F;
    std::vector<VkFence> m_renderFences_F;
    

    // Command Pools and Buffers
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers_F;

    // Immediate rendering resources
    VkFence m_immediateFence;
    VkCommandBuffer m_immediateCommandBuffer;
    VkCommandPool m_immediateCommandPool;

    // Cleanup
    DeletionQueue m_mainDeletionQueue; // Contains all deletable vulkan resources except pipelines/pipeline layouts

    void create_instance();
    void create_debug_messenger();
    void create_surface(SDL_Window * const window);
    void init_physical_device();
    void find_queue_family_indices();
    void create_device();
    void create_swapChain();
    void get_swapChain_images();
    // void create_draw_image();
    void create_depth_image_and_view();
    void create_synchronization_structures();
    void create_command_pool();
    void create_command_buffers();
    void retrieve_queues();
    void init_VMA();
public:
    void init(SDL_Window * const window);
    VkFormat m_swapChainFormat;
    AllocatedImage m_depthImage; // TODO: gfxdevice shouldnt own this
    std::vector<VkImage> m_swapChainImages; // TODO: better interface for this
    std::vector<VkImageView> m_swapChainImageViews; // TODO: better interface for this
    VkSwapchainKHR m_swapChain;
    [[nodiscard]] operator VkDevice() const
    {
        return m_device;
    }
    VkInstance get_instance() const;
    VkQueue get_graphics_queue() const;
    VkPhysicalDevice get_physical_device() const;
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) const;


    VkCommandBuffer get_frame_command_buffer(uint32_t currentFrameIndex) const;
    VkSemaphore get_frame_imageAvailableSemaphore(uint32_t currentFrameIndex) const;
    VkSemaphore get_frame_renderFinishedSemaphore(uint32_t currentFrameIndex) const;
    VkFence get_frame_fence(uint32_t currentFrameIndex) const;

    void cleanup();
};
