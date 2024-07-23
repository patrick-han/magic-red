#include <Mesh/RenderObject.h>
#include <vulkan/vulkan.h>
#include <Pipeline/GraphicsPipeline.h>
#include <glm/glm.hpp>
#include <Mesh/MeshCache.h>
#include <Pipeline/PipelineCache.h>

RenderObject::RenderObject(const GPUMeshId _GPUmeshId, const GraphicsPipelineCache& _pipelineCache, const MeshCache& _meshCache) : 
    // m_pipelineId(_pipelineId),
    m_GPUmeshId(_GPUmeshId),
    m_meshCache(_meshCache),
    m_materialId(m_meshCache.get_mesh(m_GPUmeshId).m_materialId),
    m_pipelineCache(_pipelineCache)
{}

void RenderObject::set_transform(glm::mat4 _transformMatrix) {
    m_transformMatrix = _transformMatrix;
}

void RenderObject::bind_and_draw(VkCommandBuffer commandBuffer, [[maybe_unused]] std::span<VkDescriptorSet const> descriptorSets, [[maybe_unused]] VkDeviceAddress sceneDataBufferAddress) const {

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(m_meshCache.get_mesh(m_GPUmeshId).vertexBuffer.buffer), &offset);
    vkCmdBindIndexBuffer(commandBuffer, m_meshCache.get_mesh(m_GPUmeshId).indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_meshCache.get_mesh(m_GPUmeshId).indexCount), 1, 0, 0, 0);
}
