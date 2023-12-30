#pragma once
#include "vulkan/vulkan.h"
#include <glm/glm.hpp>
#include "MeshPushConstants.h"

class Mesh;
class GraphicsPipeline;

struct RenderObject {
    RenderObject(const char* materialName, const char* meshName);
    void BindAndDraw(VkCommandBuffer commandBuffer, glm::mat4 viewProjectionMatrix) const;
    Mesh* mesh;
    GraphicsPipeline* material;
    glm::mat4 transformMatrix;
};