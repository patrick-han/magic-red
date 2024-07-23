#pragma once
#include <Mesh/DefaultPushConstants.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Common/IdTypes.h>
#include <Common/Config.h>
#include <Rendering/StageBase.h>
#include <array>
class GfxDevice;
class GraphicsPipelineCache;

class GBufferStage final : public StageBase {

    inline static constexpr std::array<VkPushConstantRange, 1> m_pushConstantRanges = {DefaultPushConstants::range()};

public:
    // GBufferStage() = delete;
    GBufferStage(
        const GfxDevice& _gfxDevice,
        GraphicsPipelineCache&  _graphicsPipelineCache,
        const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
        std::span<VkDescriptorSetLayout const> _descriptorSetLayouts
    );
    ~GBufferStage();
    GBufferStage(const GBufferStage&) = delete;
    GBufferStage& operator=(const GBufferStage&) = delete;

private:
    // const GfxDevice& m_gfxDevice;
    GraphicsPipelineCache& m_graphicsPiplineCache;
    const VkPipelineRenderingCreateInfoKHR* m_pipelineRenderingCreateInfo;
    const std::string m_vertexShaderPath = std::string("Shaders/triangle_mesh.vert.spv");
    const std::string m_fragmentShaderPath = std::string("Shaders/blinn-phong.frag.spv");
    std::span<VkDescriptorSetLayout const> m_descriptorSetLayouts;
    const VkExtent2D m_extent = {WINDOW_WIDTH, WINDOW_HEIGHT};
public:
    const GraphicsPipeline m_pipeline;
    const GraphicsPipelineId m_pipelineId;
};