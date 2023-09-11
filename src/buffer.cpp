#include "Buffer.h"
#include <vulkan/vk_enum_string_helper.h>

void upload_buffer(AllocatedBuffer& allocatedBuffer, size_t bufferSize, const void* bufferData, VkBufferUsageFlags bufferUsage, VmaAllocator allocator, DeletionQueue& deletionQueue) {

    assert(bufferSize > 0);

    
    // Create buffer
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = bufferSize;
    bufferCreateInfo.usage = bufferUsage;

    // Let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    // Allocate the buffer
    VkResult res = vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaAllocInfo,
        &reinterpret_cast<VkBuffer &>(allocatedBuffer.buffer), // bleck, but apparently "vulkan.hpp contains static_asserts that ensure that vk::Buffer and VkBuffer are the same size" so this is okay
        &allocatedBuffer.allocation,
        nullptr
    );
    if (res != VK_SUCCESS) {
        MRCERR(string_VkResult(res));
        throw std::runtime_error("Could not allocate buffer!");
    }
    
    // Add the destruction of mesh buffer to the deletion queue
    deletionQueue.push_function([=]() {
        vmaDestroyBuffer(allocator, allocatedBuffer.buffer, allocatedBuffer.allocation);
    });

    // Copy vertex data into mapped memory
    void* data;
    vmaMapMemory(allocator, allocatedBuffer.allocation, &data);
    memcpy(data, bufferData, bufferSize);
    vmaUnmapMemory(allocator, allocatedBuffer.allocation);
}