#include "VertexDescriptors.h"
#include "Mesh/Vertex.h"

[[nodiscard]] VertexInputDescription VertexInputDescription::get_default_vertex_description() {
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