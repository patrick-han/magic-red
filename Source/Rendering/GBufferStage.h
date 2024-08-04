#pragma once
#include <Mesh/DefaultPushConstants.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Common/IdTypes.h>
#include <Common/Config.h>
#include <Rendering/StageBase.h>
#include <array>

class GfxDevice;
struct RenderObject;

class GBufferStage final : public StageBase {

    inline static constexpr std::array<VkPushConstantRange, 1> m_pushConstantRanges = {DefaultPushConstants::range()};

public:
    // GBufferStage() = delete;
    GBufferStage(
        const GfxDevice& _gfxDevice,
        const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
        const VkDescriptorSetLayout _bindlessDescriptorSetLayout,
        const VkDescriptorSet _bindlessDescriptorSet
    );
    ~GBufferStage();
    GBufferStage(const GBufferStage&) = delete;
    GBufferStage& operator=(const GBufferStage&) = delete;

    void Draw(VkCommandBuffer cmdBuffer, VkDeviceAddress sceneDataBufferAddress, std::span<RenderObject> renderObjects);
    void Cleanup() override;

private:
    const std::string m_vertexShaderPath = std::string("Shaders/triangle_mesh.vert.spv");
    const std::string m_fragmentShaderPath = std::string("Shaders/gbuffer.frag.spv");
    const VkDescriptorSet m_bindlessDescriptorSet;
    const VkExtent2D m_extent = {WINDOW_WIDTH, WINDOW_HEIGHT};
public:
    GraphicsPipeline m_pipeline;
    GraphicsPipelineId m_pipelineId;
};