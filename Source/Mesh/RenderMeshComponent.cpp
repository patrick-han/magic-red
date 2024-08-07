#include <Mesh/RenderMeshComponent.h>
#include <vulkan/vulkan.h>
#include <Pipeline/GraphicsPipeline.h>
#include <glm/glm.hpp>
#include <Mesh/MeshCache.h>

RenderMeshComponent::RenderMeshComponent(const GPUMeshId _GPUmeshId, const MeshCache& _meshCache, glm::mat4 _transformMatrix) :
    m_GPUmeshId(_GPUmeshId)
    , m_meshCache(_meshCache)
    , m_transformMatrix(_transformMatrix)
    , m_materialId(m_meshCache.get_mesh(m_GPUmeshId).m_materialId)
{}

void RenderMeshComponent::bind_mesh_buffers_and_draw(VkCommandBuffer commandBuffer, [[maybe_unused]] std::span<VkDescriptorSet const> descriptorSets) const {

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(m_meshCache.get_mesh(m_GPUmeshId).vertexBuffer.buffer), &offset);
    vkCmdBindIndexBuffer(commandBuffer, m_meshCache.get_mesh(m_GPUmeshId).indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_meshCache.get_mesh(m_GPUmeshId).indexCount), 1, 0, 0, 0);
}
