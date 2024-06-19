#include <Mesh/RenderObject.h>
#include <vulkan/vulkan.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Mesh/Mesh.h>
#include <glm/glm.hpp>
#include <Pipeline/MaterialFunctions.h>
#include <Scene/Scene.h>
#include <Mesh/MeshCache.h>
#include <Pipeline/PipelineCache.h>

RenderObject::RenderObject(GraphicsPipelineId _pipelineId, MeshId _meshId, const GraphicsPipelineCache& _pipelineCache, const MeshCache& _meshCache) : 
    m_pipelineId(_pipelineId),
    m_meshId(_meshId),
    m_pipelineCache(_pipelineCache),
    m_meshCache(_meshCache)
{}

void RenderObject::set_transform(glm::mat4 _transformMatrix) {
    m_transformMatrix = _transformMatrix;
}

void RenderObject::bind_and_draw(VkCommandBuffer commandBuffer, const glm::mat4& viewProjectionMatrix, std::span<VkDescriptorSet const> descriptorSets) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineCache.get_pipeline(m_pipelineId).getPipeline());

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(m_meshCache.get_mesh(m_meshId).vertexBuffer.buffer), &offset);
    vkCmdBindIndexBuffer(commandBuffer, m_meshCache.get_mesh(m_meshId).indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Compute MVP matrix
    MeshPushConstants meshPushConstants;
    meshPushConstants.modelViewProjection = viewProjectionMatrix * m_transformMatrix;
    meshPushConstants.model = m_transformMatrix;

    vkCmdPushConstants(commandBuffer, m_pipelineCache.get_pipeline(m_pipelineId).getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(meshPushConstants), &meshPushConstants);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineCache.get_pipeline(m_pipelineId).getPipelineLayout(), 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_meshCache.get_mesh(m_meshId).indexCount), 1, 0, 0, 0);
}
