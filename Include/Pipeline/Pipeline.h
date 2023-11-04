#pragma once
#include "vulkan/vulkan.h"
#include <vector>

class Pipeline {
public:
    Pipeline();
    Pipeline(const VkDevice logicalDevice);
    virtual ~Pipeline() = 0; // Prevents direct creation of this object "pure virtual destructor"
    void Destroy();
    const VkPipeline& getPipeline() const;
    const VkPipelineLayout& getPipelineLayout() const;

protected:

    void CreatePipelineLayout(const std::vector<VkPushConstantRange>& pushConstantRanges);

    VkDevice m_logicalDevice;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
};