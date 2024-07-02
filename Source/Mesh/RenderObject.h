#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "DefaultPushConstants.h"
#include <span>
#include <Common/IdTypes.h>

class GraphicsPipeline;

class MeshCache;
class GraphicsPipelineCache;

struct RenderObject {
    RenderObject(GraphicsPipelineId _pipelineId, const GPUMeshId _meshId, const MaterialId _materialId, const GraphicsPipelineCache& _pipelineCache, const MeshCache& _meshCache);
    void bind_and_draw(VkCommandBuffer commandBuffer, std::span<VkDescriptorSet const> descriptorSets, VkDeviceAddress sceneDataBufferAddress) const;
    void set_transform(glm::mat4 _transformMatrix);

private:
    glm::mat4 m_transformMatrix;
    GraphicsPipelineId m_pipelineId;
    GPUMeshId m_meshId;
    MaterialId m_materialId;
    const GraphicsPipelineCache &m_pipelineCache;
    const MeshCache &m_meshCache;
    
};
