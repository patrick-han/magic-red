#include <Mesh/RenderObject.h>
#include <vulkan/vulkan.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Mesh/Mesh.h>
#include <glm/glm.hpp>
#include <Pipeline/MaterialFunctions.h>
#include <Scene/Scene.h>

RenderObject::RenderObject(const char* materialName, const char* meshName) { // TODO: Want to rethink the design of this + mesh loading because model loading seems all too opaque in the wrong ways, but ok for now
    material = get_material(materialName, Scene::GetInstance().sceneMaterialMap);
    mesh = get_mesh(meshName, Scene::GetInstance().sceneMeshMap);
}

void RenderObject::BindAndDraw(VkCommandBuffer commandBuffer, const glm::mat4& viewProjectionMatrix, std::span<VkDescriptorSet const> descriptorSets) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipeline());

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(mesh->vertexBuffer.buffer), &offset);
    vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Compute MVP matrix
    MeshPushConstants meshPushConstants;
    meshPushConstants.modelViewProjection = viewProjectionMatrix * transformMatrix;
    meshPushConstants.model = transformMatrix;

    vkCmdPushConstants(commandBuffer, material->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(meshPushConstants), &meshPushConstants);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(), 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
}