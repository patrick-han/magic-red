#include "Mesh/RenderMesh.h"
#include "vulkan/vulkan.h"
#include "Pipeline/GraphicsPipeline.h"
#include "Mesh/Mesh.h"
#include <glm/glm.hpp>

void RenderMesh::BindAndDraw(VkCommandBuffer commandBuffer, glm::mat4 viewProjectionMatrix) const {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipeline());

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(mesh->vertexBuffer.buffer), &offset);

    // Compute MVP matrix
    MeshPushConstants meshPushConstants;
    meshPushConstants.modelViewProjection = viewProjectionMatrix * transformMatrix;

    vkCmdPushConstants(commandBuffer, material->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(meshPushConstants), &meshPushConstants);

    vkCmdDraw(commandBuffer, mesh->vertices.size(), 1, 0, 0);
}