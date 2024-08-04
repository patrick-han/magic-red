#pragma once
#include <Mesh/DefaultPushConstants.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Common/IdTypes.h>
#include <Common/Config.h>
#include <Rendering/StageBase.h>
#include <array>

class GfxDevice;
class GraphicsPipelineCache;
struct RenderObject;

class GBufferStage final : public StageBase {

    inline static constexpr std::array<VkPushConstantRange, 1> m_pushConstantRanges = {DefaultPushConstants::range()};

public:
    // GBufferStage() = delete;
    GBufferStage(
        const GfxDevice& _gfxDevice,
        GraphicsPipelineCache&  _graphicsPipelineCache,
        const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
        std::span<VkDescriptorSetLayout const> _descriptorSetLayouts,
        std::span<VkDescriptorSet const> _descriptorSets
    );
    ~GBufferStage();
    GBufferStage(const GBufferStage&) = delete;
    GBufferStage& operator=(const GBufferStage&) = delete;

    virtual void Draw(VkCommandBuffer cmdBuffer, VkDeviceAddress sceneDataBufferAddress, std::span<RenderObject> renderObjects) override;

private:
    GraphicsPipelineCache& m_graphicsPipelineCache;
    const std::string m_vertexShaderPath = std::string("Shaders/triangle_mesh.vert.spv");
    const std::string m_fragmentShaderPath = std::string("Shaders/gbuffer.frag.spv");
    std::span<VkDescriptorSetLayout const> m_descriptorSetLayouts;
    std::span<VkDescriptorSet const> m_descriptorSets;
    const VkExtent2D m_extent = {WINDOW_WIDTH, WINDOW_HEIGHT};
public:
    GraphicsPipeline m_pipeline;
    GraphicsPipelineId m_pipelineId;
};