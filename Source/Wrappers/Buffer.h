#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct DeletionQueue;

/* Given the raw desired data, upload a buffer to the GPU */
void upload_buffer(AllocatedBuffer& allocatedBuffer, size_t bufferSize, const void* bufferData, VkBufferUsageFlags bufferUsage, VmaAllocator allocator, DeletionQueue& deletionQueue);

/* Update a buffer's data */
void update_buffer(AllocatedBuffer& allocatedBuffer, size_t bufferSize, const void* bufferData, VmaAllocator allocator);