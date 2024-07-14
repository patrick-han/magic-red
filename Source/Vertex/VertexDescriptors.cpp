#include <Vertex/VertexDescriptors.h>
#include <Vertex/Vertex.h>

[[nodiscard]] VertexInputDescription VertexInputDescription::get_default_vertex_description() {
    VertexInputDescription description;

    // We will have 1 vertex buffer binding, with a per vertex rate (rather than instance)
    VkVertexInputBindingDescription mainBindingDescription = {};
    mainBindingDescription.binding = 0;
    mainBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    mainBindingDescription.stride = sizeof(Vertex);

    description.bindings.push_back(mainBindingDescription);

    VkVertexInputAttributeDescription positionAttribute = {};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset = offsetof(Vertex, position);

    VkVertexInputAttributeDescription uv_xAttribute = {};
    uv_xAttribute.binding = 0;
    uv_xAttribute.location = 1;
    uv_xAttribute.format = VK_FORMAT_R32_SFLOAT;
    uv_xAttribute.offset = offsetof(Vertex, uv_x);

    VkVertexInputAttributeDescription normalAttribute = {};
    normalAttribute.binding = 0;
    normalAttribute.location = 2;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(Vertex, normal);

    VkVertexInputAttributeDescription uv_yAttribute = {};
    uv_yAttribute.binding = 0;
    uv_yAttribute.location = 3;
    uv_yAttribute.format = VK_FORMAT_R32_SFLOAT;
    uv_yAttribute.offset = offsetof(Vertex, uv_y);

    VkVertexInputAttributeDescription tangentAttribute = {};
    tangentAttribute.binding = 0;
    tangentAttribute.location = 4;
    tangentAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    tangentAttribute.offset = offsetof(Vertex, tangent);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(uv_xAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(uv_yAttribute);
    description.attributes.push_back(tangentAttribute);
    return description;
}