#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "MeshPushConstants.h"
#include <span>

struct Mesh;
class GraphicsPipeline;

struct RenderObject {
    RenderObject(GraphicsPipeline* _material, Mesh* _mesh);
    void BindAndDraw(VkCommandBuffer commandBuffer, const glm::mat4& viewProjectionMatrix, std::span<VkDescriptorSet const> descriptorSets) const;
    Mesh* mesh;
    GraphicsPipeline* material;
    glm::mat4 transformMatrix;
};