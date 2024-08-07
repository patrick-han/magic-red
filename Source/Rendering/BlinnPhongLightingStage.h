#pragma once
#include <Mesh/DefaultPushConstants.h>
#include <Pipeline/GraphicsPipeline.h>
#include <Common/IdTypes.h>
#include <Common/Config.h>
#include <Rendering/StageBase.h>
#include <array>
#include <Common/IdTypes.h>

class GfxDevice;
class TextureCache;

struct DescriptorSetLayoutBinding {
    VkDescriptorType descriptorType;
    int descriptorCount;
};

class BlinnPhongLightingStage final : public StageBase {

    inline static constexpr std::array<VkPushConstantRange, 1> m_pushConstantRanges = {DefaultPushConstants::range()};

public:
    // BlinnPhongLightingStage() = delete;
    BlinnPhongLightingStage(
        const GfxDevice& _gfxDevice,
        const VkPipelineRenderingCreateInfoKHR* _pipelineRenderingCreateInfo,
        const TextureCache& _textureCache,
        const VkDescriptorPool _globalDescriptorPool,
        const VkDescriptorSetLayout _bindlessDescriptorSetLayout,
        const VkDescriptorSet _bindlessDescriptorSet,
        GPUTextureId _albedoRTId,
        GPUTextureId _worldNormalsRTId,
        GPUTextureId _metallicRoughnessRTId
    );
    ~BlinnPhongLightingStage();
    BlinnPhongLightingStage(const BlinnPhongLightingStage&) = delete;
    BlinnPhongLightingStage& operator=(const BlinnPhongLightingStage&) = delete;

    void Draw(VkCommandBuffer cmdBuffer, VkDeviceAddress sceneDataBufferAddress);
    void Cleanup() override;

private:
    const std::string m_vertexShaderPath = std::string("Shaders/fullscreen_quad.vert.spv");
    const std::string m_fragmentShaderPath = std::string("Shaders/blinn-phong.frag.spv");
    const VkExtent2D m_extent = {WINDOW_WIDTH, WINDOW_HEIGHT};

    const TextureCache& m_textureCache;
    const VkDescriptorPool m_globalDescriptorPool;
    const VkDescriptorSet m_bindlessDescriptorSet;
    VkDescriptorSetLayout m_lightingDescriptorSetLayout;
    VkDescriptorSet m_lightingDescriptorSet;
public:
    GraphicsPipeline m_pipeline;
    GraphicsPipelineId m_pipelineId;
};
