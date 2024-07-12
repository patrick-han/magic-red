#include <Wrappers/Buffer.h>
#include <vulkan/vk_enum_string_helper.h> // Doesn't work on linux?
#include <cassert>
#include <Common/Log.h>
#include <cstring>

void upload_buffer(AllocatedBuffer& allocatedBuffer, size_t bufferSize, const void* bufferData, VkBufferUsageFlags bufferUsage, VmaAllocator allocator) {

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
        &allocatedBuffer.buffer,
        &allocatedBuffer.allocation,
        nullptr
    );
    if (res != VK_SUCCESS) {
        MRCERR(string_VkResult(res));
        MRCERR("Could not allocate buffer!");
    }

    // Copy data into mapped memory
    void* data;
    vmaMapMemory(allocator, allocatedBuffer.allocation, &data);
    memcpy(data, bufferData, bufferSize);
    vmaUnmapMemory(allocator, allocatedBuffer.allocation);
}

void update_buffer(AllocatedBuffer& allocatedBuffer, size_t bufferSize, const void* bufferData, VmaAllocator allocator) {

    assert(bufferSize > 0);

    // Copy vertex data into mapped memory
    void* data;
    vmaMapMemory(allocator, allocatedBuffer.allocation, &data);
    memcpy(data, bufferData, bufferSize);
    vmaUnmapMemory(allocator, allocatedBuffer.allocation);
}

void AllocatedBuffer::cleanup(VmaAllocator allocator) {
    vmaDestroyBuffer(allocator, buffer, allocation);
}
