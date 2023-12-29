#pragma once
#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

/* Generate a sensible default image create info */
VkImageCreateInfo image_create_info(VkFormat format, VkExtent3D extent, VkImageUsageFlags usageFlags);

/* Generate a sensible default image view create info */
VkImageViewCreateInfo imageview_create_info(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);