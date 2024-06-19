#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "MeshPushConstants.h"
#include <span>
#include <Common/IdTypes.h>

class GraphicsPipeline;

 class MeshCache;

struct RenderObject {
    RenderObject(GraphicsPipeline* _material, MeshId _meshId, const MeshCache& _meshCache);
    void bind_and_draw(VkCommandBuffer commandBuffer, const glm::mat4& viewProjectionMatrix, std::span<VkDescriptorSet const> descriptorSets) const;
    void set_transform(glm::mat4 _transformMatrix);

private:
    glm::mat4 m_transformMatrix;
    GraphicsPipeline* material;
    MeshId m_meshId;
    const MeshCache &m_meshCache;
};
