#pragma once
#include <vulkan/vulkan.h>

[[nodiscard]] VkImageMemoryBarrier image_memory_barrier(
    VkImage image,
    VkAccessFlags srcAcessMask,
    VkAccessFlags dstAccesMask,
    VkImageLayout oldLayout,
    VkImageLayout newLayout
    );