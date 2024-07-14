#include <Mesh/RenderObject.h>
#include <vulkan/vulkan.h>
#include <Pipeline/GraphicsPipeline.h>
#include <glm/glm.hpp>
#include <Mesh/MeshCache.h>
#include <Pipeline/PipelineCache.h>

RenderObject::RenderObject(GraphicsPipelineId _pipelineId, const GPUMeshId _GPUmeshId, const GraphicsPipelineCache& _pipelineCache, const MeshCache& _meshCache) : 
    m_pipelineId(_pipelineId),
    m_GPUmeshId(_GPUmeshId),
    m_pipelineCache(_pipelineCache),
    m_meshCache(_meshCache),
    m_materialId(m_meshCache.get_mesh(m_GPUmeshId).m_materialId)
{}

void RenderObject::set_transform(glm::mat4 _transformMatrix) {
    m_transformMatrix = _transformMatrix;
}

void RenderObject::bind_and_draw(VkCommandBuffer commandBuffer, [[maybe_unused]] std::span<VkDescriptorSet const> descriptorSets, VkDeviceAddress sceneDataBufferAddress) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineCache.get_pipeline(m_pipelineId).get_pipeline());

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(m_meshCache.get_mesh(m_GPUmeshId).vertexBuffer.buffer), &offset);
    vkCmdBindIndexBuffer(commandBuffer, m_meshCache.get_mesh(m_GPUmeshId).indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Compute MVP matrix
    DefaultPushConstants pushConstants;
    pushConstants.model = m_transformMatrix;
    pushConstants.sceneDataBufferAddress = sceneDataBufferAddress;
    pushConstants.materialId = m_materialId;
    

    vkCmdPushConstants(commandBuffer, m_pipelineCache.get_pipeline(m_pipelineId).get_pipeline_layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineCache.get_pipeline(m_pipelineId).get_pipeline_layout(), 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_meshCache.get_mesh(m_GPUmeshId).indexCount), 1, 0, 0, 0);
}
