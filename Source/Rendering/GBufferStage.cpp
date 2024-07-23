#include "GBufferStage.h"
#include <Rendering/GfxDevice.h>
#include <Pipeline/PipelineCache.h>
#include <Common/Compiler/Unused.h>
#include <Mesh/RenderObject.h>
#include <Common/Defaults.h>

GBufferStage::GBufferStage(
    const GfxDevice& _gfxDevice,
    GraphicsPipelineCache&  _graphicsPipelineCache,
    const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
    std::span<VkDescriptorSetLayout const> _descriptorSetLayouts,
    // std::span<VkDescriptorSet* const> _descriptorSets, 
    VkDescriptorSet* _pDescriptorSet)
    : StageBase(_gfxDevice)
    , m_graphicsPipelineCache(_graphicsPipelineCache)
    , m_pipelineRenderingCreateInfo(_pipelineRenderingCreateInfo)
    , m_descriptorSetLayouts(_descriptorSetLayouts) 
    // , m_descriptorSets(_descriptorSets)
    , m_pDescriptorSet(_pDescriptorSet)
    , m_pipeline(m_gfxDevice, m_pipelineRenderingCreateInfo, m_vertexShaderPath, m_fragmentShaderPath, m_pushConstantRanges, m_descriptorSetLayouts, m_extent)
    , m_pipelineId(m_graphicsPipelineCache.add_pipeline(m_gfxDevice, m_pipeline)) {

    }

GBufferStage::~GBufferStage() {}

void GBufferStage::Draw(VkCommandBuffer cmdBuffer, VkDeviceAddress sceneDataBufferAddress, std::span<RenderObject> renderObjects) {

    vkCmdSetViewport(cmdBuffer, 0, 1, &DEFAULT_VIEWPORT_FULLSCREEN);
    vkCmdSetScissor(cmdBuffer, 0, 1, &DEFAULT_SCISSOR_FULLSCREEN);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get_pipeline_handle());

    // Bindless descriptor set shared for color pass
    //vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
    //    m_pipeline.get_pipeline_layout(), 
    //    0, static_cast<uint32_t>(m_descriptorSets.size()), *m_descriptorSets.data(), 0, nullptr); // TODO: Hardcoded
     vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
         m_pipeline.get_pipeline_layout(), 
         0, 1, m_pDescriptorSet, 0, nullptr); // TODO: Hardcoded


    for(const RenderObject& renderObject : renderObjects)
    {
        DefaultPushConstants pushConstants;
        pushConstants.model = renderObject.m_transformMatrix;
        pushConstants.sceneDataBufferAddress = sceneDataBufferAddress;
        pushConstants.materialId = renderObject.m_materialId;
        vkCmdPushConstants(cmdBuffer, m_pipeline.get_pipeline_layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        renderObject.bind_mesh_buffers_and_draw(cmdBuffer, std::span<const VkDescriptorSet>());
    }
}