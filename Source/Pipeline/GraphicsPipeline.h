#pragma once
#include "Pipeline/Pipeline.h"

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline();
    GraphicsPipeline(
        const VkDevice logicalDevice,
        const VkPipelineRenderingCreateInfoKHR* pipelineRenderingCreateInfo,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath, 
        const std::vector<VkPushConstantRange>& pushConstantRanges,
        VkExtent2D extent
        );
    ~GraphicsPipeline();
protected:

};