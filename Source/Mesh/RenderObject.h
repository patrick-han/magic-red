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
    void bind_mesh_buffers_and_draw(VkCommandBuffer commandBuffer, std::span<VkDescriptorSet const> descriptorSets) const;
    void set_transform(glm::mat4 _transformMatrix);


    
private:
    const GraphicsPipelineCache &m_pipelineCache;
    GPUMeshId m_GPUmeshId;
    const MeshCache &m_meshCache;
public:
    glm::mat4 m_transformMatrix;
    MaterialId m_materialId;
};
