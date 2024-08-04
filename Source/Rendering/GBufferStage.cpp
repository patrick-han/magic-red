#include "GBufferStage.h"
#include <Rendering/GfxDevice.h>
#include <Mesh/RenderObject.h>
#include <Common/Defaults.h>

GBufferStage::GBufferStage(
    const GfxDevice& _gfxDevice,
    const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
    VkDescriptorSetLayout _bindlessDescriptorSetLayout,
    VkDescriptorSet _bindlessDescriptorSet)
    : StageBase(_gfxDevice)
    , m_bindlessDescriptorSet(_bindlessDescriptorSet)
    , m_pipeline(m_gfxDevice)
    {

        VertexInputDescription vertexDescription = VertexInputDescription::get_default_vertex_description();
        std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {{_bindlessDescriptorSetLayout}};
        m_pipeline.BuildPipeline(
            _pipelineRenderingCreateInfo
            , m_vertexShaderPath, m_fragmentShaderPath
            , vertexDescription
            , m_pushConstantRanges
            , descriptorSetLayouts
            , m_extent
            );
    }

GBufferStage::~GBufferStage() {}

void GBufferStage::Draw(VkCommandBuffer cmdBuffer, VkDeviceAddress sceneDataBufferAddress, std::span<RenderObject> renderObjects) {

    vkCmdSetViewport(cmdBuffer, 0, 1, &DEFAULT_VIEWPORT_FULLSCREEN);
    vkCmdSetScissor(cmdBuffer, 0, 1, &DEFAULT_SCISSOR_FULLSCREEN);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get_pipeline_handle());

    // Bindless descriptor set shared for color pass
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
       m_pipeline.get_pipeline_layout(), 
       0, 1, &m_bindlessDescriptorSet, 0, nullptr);

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

void GBufferStage::Cleanup() {
    vkDestroyPipelineLayout(m_gfxDevice, m_pipeline.get_pipeline_layout(), nullptr);
    vkDestroyPipeline(m_gfxDevice, m_pipeline.get_pipeline_handle(), nullptr);
}