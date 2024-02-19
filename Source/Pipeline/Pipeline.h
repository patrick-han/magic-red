#pragma once
#include <vulkan/vulkan.h>
#include <span>

class Pipeline {
public:
    Pipeline();
    Pipeline(const VkDevice logicalDevice);
    virtual ~Pipeline() = 0; // Prevents direct creation of this object "pure virtual destructor"
    void Destroy();
    const VkPipeline& getPipeline() const;
    const VkPipelineLayout& getPipelineLayout() const;

protected:

    void CreatePipelineLayout(
        std::span<VkPushConstantRange const> pushConstantRanges, 
        std::span<VkDescriptorSetLayout const> descriptorSetLayouts
        );

    VkDevice m_logicalDevice;
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
};