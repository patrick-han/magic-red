#include "ImageMemoryBarrier.h"

[[nodiscard]] VkImageMemoryBarrier image_memory_barrier(
    VkImage image,
    VkAccessFlags srcAcessMask,
    VkAccessFlags dstAccesMask,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageAspectFlags aspectFlags
    ) {
    VkImageMemoryBarrier imb = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = srcAcessMask,
        .dstAccessMask = dstAccesMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    return imb;
}