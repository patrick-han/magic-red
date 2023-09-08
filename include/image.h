#pragma once
#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.h"

struct AllocatedImage {
    vk::Image image;
    VmaAllocation allocation;
};