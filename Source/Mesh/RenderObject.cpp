#include <Mesh/RenderObject.h>
#include <vulkan/vulkan.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Mesh/Mesh.h>
#include <glm/glm.hpp>
#include <Pipeline/MaterialFunctions.h>
#include <Scene/Scene.h>
#include <Mesh/MeshCache.h>

RenderObject::RenderObject(GraphicsPipeline* _material, MeshId _meshId, const MeshCache& _meshCache) : 
    material(_material),
    m_meshId(_meshId),
    m_meshCache(_meshCache)
{}

void RenderObject::set_transform(glm::mat4 _transformMatrix) {
    m_transformMatrix = _transformMatrix;
}

void RenderObject::BindAndDraw(VkCommandBuffer commandBuffer, const glm::mat4& viewProjectionMatrix, std::span<VkDescriptorSet const> descriptorSets) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipeline());

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(m_meshCache.get_mesh(m_meshId).vertexBuffer.buffer), &offset);
    vkCmdBindIndexBuffer(commandBuffer, m_meshCache.get_mesh(m_meshId).indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Compute MVP matrix
    MeshPushConstants meshPushConstants;
    meshPushConstants.modelViewProjection = viewProjectionMatrix * m_transformMatrix;
    meshPushConstants.model = m_transformMatrix;

    vkCmdPushConstants(commandBuffer, material->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(meshPushConstants), &meshPushConstants);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(), 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_meshCache.get_mesh(m_meshId).indexCount), 1, 0, 0, 0);
}
