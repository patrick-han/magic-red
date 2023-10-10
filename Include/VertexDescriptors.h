#include <vector>
#include "vulkan/vulkan.hpp"

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

    /* Return VertexInputBinding and VertexInputAttribute descriptions for the Vertex type */
    static VertexInputDescription get_default_vertex_description();
};
