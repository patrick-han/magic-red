#pragma once
#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.h"
#include "types.h"

struct AllocatedBuffer {
    vk::Buffer buffer;
    VmaAllocation allocation;
};

/* Given the raw desired data, upload a buffer to the GPU */
void upload_buffer(AllocatedBuffer& allocatedBuffer, size_t bufferSize, const void* bufferData, VkBufferUsageFlags bufferUsage, VmaAllocator allocator, DeletionQueue& deletionQueue);
