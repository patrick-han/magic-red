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
    RenderObject(const GPUMeshId _GPUmeshId, const GraphicsPipelineCache& _pipelineCache, const MeshCache& _meshCache);
    void bind_and_draw(VkCommandBuffer commandBuffer, std::span<VkDescriptorSet const> descriptorSets, VkDeviceAddress sceneDataBufferAddress) const;
    void set_transform(glm::mat4 _transformMatrix);


    
private:
    GPUMeshId m_GPUmeshId;
    const MeshCache &m_meshCache;
public:
    MaterialId m_materialId;
private:
    const GraphicsPipelineCache &m_pipelineCache;
    glm::mat4 m_transformMatrix;
};
