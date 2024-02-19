#include <Pipeline/Pipeline.h>

Pipeline::Pipeline() {
}

Pipeline::Pipeline(const VkDevice device) : m_logicalDevice(device) {
}

Pipeline::~Pipeline() {}


void Pipeline::Destroy() {
    vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
    vkDestroyPipeline(m_logicalDevice, m_pipeline, nullptr);
}

const VkPipeline& Pipeline::getPipeline() const {
    return m_pipeline;
}

const VkPipelineLayout& Pipeline::getPipelineLayout() const {
    return m_pipelineLayout;
}

void Pipeline::CreatePipelineLayout(
    std::span<VkPushConstantRange const> pushConstantRanges, 
    std::span<VkDescriptorSetLayout const> descriptorSetLayouts
    ) {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = {};
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

    vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
}
