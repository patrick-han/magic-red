#pragma once

#include "Buffer.h"
#include "Types.h"
#include <vector>
#include <glm/vec3.hpp>



struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

/* 
 * A description that includes:
 * 
 * Bindings descriptions which, in combination with pOffsets in vkCmdBindVertexBuffers, indicate where in vertex buffer(s) the vertex shader begins
 * 
 * Attribute descriptions which define vertex attributes
 */
struct VertexInputDescription {
    std::vector<vk::VertexInputBindingDescription> bindings;
    std::vector<vk::VertexInputAttributeDescription> attributes;

    vk::PipelineVertexInputStateCreateFlags flags = {};
};

/* Return VertexInputBinding and VertexInputAttribute descriptions for the Vertex type */
VertexInputDescription get_vertex_description();




struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer  indexBuffer;
};

/* Load Mesh data from an .obj file */
void load_mesh_from_obj(Mesh& mesh, const char* objFileName);

/* Upload an instantiated Mesh to the GPU using a created VMA allocator */
[[nodiscard]] Mesh& upload_mesh(Mesh& mesh, VmaAllocator allocator, DeletionQueue& deletionQueue);

/* Get a meshn from a scene mesh map */
[[nodiscard]] Mesh* get_mesh(const std::string& meshName, std::unordered_map<std::string, Mesh>& meshMap);