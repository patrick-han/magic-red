#pragma once
#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.h"

struct AllocatedBuffer {
    vk::Buffer buffer;
    VmaAllocation allocation;
};
