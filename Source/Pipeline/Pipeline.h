#pragma once
#include <vulkan/vulkan.h>
#include <span>

class Pipeline {
public:
    Pipeline() = delete;
    Pipeline(const VkDevice logicalDevice);
    virtual ~Pipeline() = 0; // Prevents direct creation of this object "pure virtual destructor"
    void destroy();
    const VkPipeline& get_pipeline() const;
    const VkPipelineLayout& get_pipeline_layout() const;

protected:

    void CreatePipelineLayout(
        std::span<VkPushConstantRange const> pushConstantRanges, 
        std::span<VkDescriptorSetLayout const> descriptorSetLayouts
        );

    VkDevice m_logicalDevice;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
};