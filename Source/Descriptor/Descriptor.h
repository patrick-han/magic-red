#pragma once
#include <vector>
#include <span>
#include "vulkan/vulkan.h"

struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(uint32_t bindingIndex, VkDescriptorType descriptorType);
    void clear();
    [[nodiscard]] VkDescriptorSetLayout buildLayout(VkDevice device, VkShaderStageFlags shaderStages);
};

struct DescriptorAllocator {
    struct PoolSizeRatio{ // Describes how many individual bindings we have of a given descriptor type
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;

    void init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio const> poolRatios);
    void clear_descriptors(VkDevice device);
    void destroy_pool(VkDevice device);
    [[nodiscard]] VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};