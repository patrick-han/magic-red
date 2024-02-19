#pragma once
#include <Pipeline/Pipeline.h>
#include <string>

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline();
    GraphicsPipeline(
        const VkDevice logicalDevice,
        const VkPipelineRenderingCreateInfoKHR* pipelineRenderingCreateInfo,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath, 
        std::span<VkPushConstantRange const> pushConstantRanges,
        std::span<VkDescriptorSetLayout const> descriptorSetLayouts,
        VkExtent2D extent
        );
    ~GraphicsPipeline();
protected:

};