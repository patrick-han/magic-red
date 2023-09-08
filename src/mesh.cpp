#include "mesh.h"
#include "vulkan/vulkan.hpp"
#include "utils.h"

VertexInputDescription get_vertex_description() {
    VertexInputDescription description;

    // We will have 1 vertex buffer binding, with a per vertex rate (rather than instance)
    vk::VertexInputBindingDescription mainBindingDescription = {};
    mainBindingDescription.binding = 0;
    mainBindingDescription.inputRate = vk::VertexInputRate::eVertex;
    mainBindingDescription.stride = sizeof(Vertex);

    description.bindings.push_back(mainBindingDescription);

    // We have 3 attributes: position, color, and normal stored at locations 0, 1, and 2
    vk::VertexInputAttributeDescription positionAttribute = {};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = vk::Format::eR32G32B32Sfloat;
    positionAttribute.offset = offsetof(Vertex, position);

    vk::VertexInputAttributeDescription normalAttribute = {};
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = vk::Format::eR32G32B32Sfloat;
    normalAttribute.offset = offsetof(Vertex, normal);

    vk::VertexInputAttributeDescription colorAttribute = {};
    colorAttribute.binding = 0;
    colorAttribute.location = 2;
    colorAttribute.format = vk::Format::eR32G32B32Sfloat;
    colorAttribute.offset = offsetof(Vertex, color);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(colorAttribute);
    return description;
}

void upload_mesh(Mesh& mesh, VmaAllocator allocator, DeletionQueue* deletionQueue) {
    // Create vertex buffer
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = mesh.vertices.size() * sizeof(Vertex);
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    // Let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    // Allocate the buffer
    VkResult res = vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaAllocInfo,
        &reinterpret_cast<VkBuffer &>(mesh.vertexBuffer.buffer), // bleck, but apparently "vulkan.hpp contains static_asserts that ensure that vk::Buffer and VkBuffer are the same size" so this is okay
        &mesh.vertexBuffer.allocation,
        nullptr
    );
    if (res != VK_SUCCESS) {
        MRCERR("Failed to allocate vertex buffer!");
        exit(0);
    }
    
    // Add the destruction of mesh buffer to the deletion queue
    deletionQueue->push_function([=]() {
        vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
    });

    // Copy vertex data into mapped memory
    void* data;
    vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);
    memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);



    // Create index buffer
    VkBufferCreateInfo bufferCreateInfo2 = {};
    bufferCreateInfo2.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo2.size = mesh.indices.size() * sizeof(uint32_t);
    bufferCreateInfo2.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    // Let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaAllocInfo2 = {};
    vmaAllocInfo2.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    // Allocate the buffer
    VkResult res2 = vmaCreateBuffer(allocator, &bufferCreateInfo2, &vmaAllocInfo2,
        &reinterpret_cast<VkBuffer &>(mesh.indexBuffer.buffer), // bleck, but apparently "vulkan.hpp contains static_asserts that ensure that vk::Buffer and VkBuffer are the same size" so this is okay
        &mesh.indexBuffer.allocation,
        nullptr
    );
    if (res2 != VK_SUCCESS) {
        MRCERR("Failed to allocate index buffer!");
        exit(0);
    }
    
    // Add the destruction of mesh buffer to the deletion queue
    deletionQueue->push_function([=]() {
        vmaDestroyBuffer(allocator, mesh.indexBuffer.buffer, mesh.indexBuffer.allocation);
    });

    // Copy vertex data into mapped memory
    void* data2;
    vmaMapMemory(allocator, mesh.indexBuffer.allocation, &data2);
    memcpy(data2, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));
    vmaUnmapMemory(allocator, mesh.indexBuffer.allocation);

}