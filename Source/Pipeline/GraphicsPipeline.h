#pragma once
#include <Pipeline/Pipeline.h>
#include <string>
#include <Vertex/VertexDescriptors.h>

class GfxDevice;

class GraphicsPipeline final : public Pipeline {
public:
    GraphicsPipeline(const GfxDevice& _gfxDevice);
    void BuildPipeline(
        const VkPipelineRenderingCreateInfoKHR* pipelineRenderingCreateInfo,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath, 
        VertexInputDescription& vertexDescription,
        std::span<VkPushConstantRange const> pushConstantRanges,
        std::span<VkDescriptorSetLayout const> descriptorSetLayouts,
        VkExtent2D extent
        );
    ~GraphicsPipeline() = default;
    // GraphicsPipeline(GraphicsPipeline&) = delete;
    // GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    // GraphicsPipeline(GraphicsPipeline&&) = delete;
    // GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;
};