#include "GBufferStage.h"
#include <Rendering/GfxDevice.h>
#include <Pipeline/PipelineCache.h>


GBufferStage::GBufferStage(
    const GfxDevice& _gfxDevice,
    GraphicsPipelineCache&  _graphicsPipelineCache,
    const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
    std::span<VkDescriptorSetLayout const> _descriptorSetLayouts) 
    // : m_gfxDevice(_gfxDevice) 
    : StageBase(_gfxDevice)
    , m_graphicsPiplineCache(_graphicsPipelineCache)
    , m_pipelineRenderingCreateInfo(_pipelineRenderingCreateInfo)
    , m_descriptorSetLayouts(_descriptorSetLayouts) 
    , m_pipeline(m_gfxDevice, m_pipelineRenderingCreateInfo, m_vertexShaderPath, m_fragmentShaderPath, m_pushConstantRanges, m_descriptorSetLayouts, m_extent)
    , m_pipelineId(m_graphicsPiplineCache.add_pipeline(m_gfxDevice, m_pipeline)) {

    }

GBufferStage::~GBufferStage() {}