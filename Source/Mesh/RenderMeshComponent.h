#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "DefaultPushConstants.h"
#include <span>
#include <Common/IdTypes.h>

class MeshCache;

struct RenderMeshComponent {
    RenderMeshComponent(const GPUMeshId _GPUmeshId, const MeshCache& _meshCache, glm::mat4 _transformMatrix);
    void bind_mesh_buffers_and_draw(VkCommandBuffer commandBuffer, std::span<VkDescriptorSet const> descriptorSets) const;


    
private:
    const GPUMeshId m_GPUmeshId;
    const MeshCache &m_meshCache;
public:
    glm::mat4 m_transformMatrix;
    MaterialId m_materialId;
};
