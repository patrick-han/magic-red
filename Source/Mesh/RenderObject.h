#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "MeshPushConstants.h"
#include <span>
#include <Common/IdTypes.h>

class GraphicsPipeline;

class MeshCache;
class GraphicsPipelineCache;

struct RenderObject {
    RenderObject(GraphicsPipelineId _pipelineId, MeshId _meshId, const GraphicsPipelineCache& _pipelineCache, const MeshCache& _meshCache);
    void bind_and_draw(VkCommandBuffer commandBuffer, const glm::mat4& viewProjectionMatrix, std::span<VkDescriptorSet const> descriptorSets) const;
    void set_transform(glm::mat4 _transformMatrix);

private:
    glm::mat4 m_transformMatrix;
    GraphicsPipelineId m_pipelineId;
    MeshId m_meshId;
    const GraphicsPipelineCache &m_pipelineCache;
    const MeshCache &m_meshCache;
};
