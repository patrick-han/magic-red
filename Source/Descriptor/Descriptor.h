#pragma once
#include <vector>
#include <span>
#include <vulkan/vulkan.h>

struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(uint32_t bindingIndex, VkDescriptorType descriptorType, uint32_t descriptorCount);
    void clear();
    [[nodiscard]] VkDescriptorSetLayout buildLayout(VkDevice device, VkShaderStageFlags shaderStages, std::span<VkDescriptorBindingFlags const> additionalLayoutBindingFlags, VkDescriptorSetLayoutCreateFlags additionalLayoutCreateFlags);
};

struct DescriptorAllocator {
    struct DescriptorTypeCount{ // Describes how many individual bindings we have of a given descriptor type
        VkDescriptorType type;
        uint32_t count;
    };

    VkDescriptorPool pool;

    void init_pool(VkDevice device, uint32_t maxSets, std::span<DescriptorTypeCount const> descriptorCounts, VkDescriptorPoolCreateFlags poolFlags);
    void clear_descriptors(VkDevice device);
    void destroy_pool(VkDevice device);
    [[nodiscard]] VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};