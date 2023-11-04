#pragma once
#include "Pipeline/Pipeline.h"

class GraphicsPipeline : public Pipeline {
public:
    GraphicsPipeline();
    GraphicsPipeline(
        const VkDevice logicalDevice,
        const VkRenderPass renderPass, 
        const std::string& vertexShaderPath, // std::string or something else?
        const std::string& fragmentShaderPath, 
        const std::vector<VkPushConstantRange>& pushConstantRanges
        );
    ~GraphicsPipeline();
protected:

};