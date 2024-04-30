#include <vector>
#include <vulkan/vulkan.h>

/* 
 * A description that includes:
 * 
 * Bindings descriptions which, in combination with pOffsets in vkCmdBindVertexBuffers, indicate where in vertex buffer(s) the vertex shader begins
 * 
 * Attribute descriptions which define vertex attributes
 */
struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = {};

    /* Return VertexInputBinding and VertexInputAttribute descriptions for the Vertex type */
    static VertexInputDescription get_default_vertex_description();
};
