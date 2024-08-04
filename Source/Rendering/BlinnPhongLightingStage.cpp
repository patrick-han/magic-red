#include "BlinnPhongLightingStage.h"
#include <Rendering/GfxDevice.h>
#include <Pipeline/PipelineCache.h>
#include <Common/Compiler/Unused.h>
#include <Mesh/RenderObject.h>
#include <Common/Defaults.h>

BlinnPhongLightingStage::BlinnPhongLightingStage(
    const GfxDevice& _gfxDevice,
    GraphicsPipelineCache&  _graphicsPipelineCache,
    const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
    std::span<VkDescriptorSetLayout const> _descriptorSetLayouts,
    std::span<VkDescriptorSet const> _descriptorSets)
    : StageBase(_gfxDevice)
    , m_graphicsPipelineCache(_graphicsPipelineCache)
    , m_descriptorSetLayouts(_descriptorSetLayouts) 
    , m_descriptorSets(_descriptorSets)
    , m_pipeline(m_gfxDevice) 
    {
      VertexInputDescription vertexDescription;
      m_pipeline.BuildPipeline(
            _pipelineRenderingCreateInfo
            , m_vertexShaderPath, m_fragmentShaderPath
            , vertexDescription
            , m_pushConstantRanges
            , m_descriptorSetLayouts
            , m_extent
            );
        m_pipelineId = m_graphicsPipelineCache.add_pipeline(_gfxDevice, m_pipeline);
    }

BlinnPhongLightingStage::~BlinnPhongLightingStage() {}

void BlinnPhongLightingStage::Draw(VkCommandBuffer cmdBuffer, VkDeviceAddress sceneDataBufferAddress, std::span<RenderObject> renderObjects) {

    UNUSED(renderObjects);

    vkCmdSetViewport(cmdBuffer, 0, 1, &DEFAULT_VIEWPORT_FULLSCREEN);
    vkCmdSetScissor(cmdBuffer, 0, 1, &DEFAULT_SCISSOR_FULLSCREEN);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get_pipeline_handle());

    // Bindless descriptor set shared for color pass
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
      m_pipeline.get_pipeline_layout(), 
      0, static_cast<uint32_t>(m_descriptorSets.size()), m_descriptorSets.data(), 0, nullptr); // TODO: Hardcoded


    // TODO: different push constants template, just using this for the sceneBuffer for now
    DefaultPushConstants pushConstants;
    pushConstants.model = glm::mat4(0.0f);
    pushConstants.sceneDataBufferAddress = sceneDataBufferAddress;
    pushConstants.materialId = 0;
    vkCmdPushConstants(cmdBuffer, m_pipeline.get_pipeline_layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
    vkCmdDraw(cmdBuffer, 6, 1, 0, 0);

    // for(const RenderObject& renderObject : renderObjects)
    // {
    //     DefaultPushConstants pushConstants;
    //     pushConstants.model = renderObject.m_transformMatrix;
    //     pushConstants.sceneDataBufferAddress = sceneDataBufferAddress;
    //     pushConstants.materialId = renderObject.m_materialId;
    //     vkCmdPushConstants(cmdBuffer, m_pipeline.get_pipeline_layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

    //     renderObject.bind_mesh_buffers_and_draw(cmdBuffer, std::span<const VkDescriptorSet>());
    // }
}