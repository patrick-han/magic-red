#pragma once
#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

struct AllocatedImage {
    VkImage image;
    VmaAllocation allocation;
};