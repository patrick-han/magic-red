#include "Descriptor.h"

void DescriptorLayoutBuilder::add_binding(uint32_t bindingIndex, VkDescriptorType descriptorType) {
    VkDescriptorSetLayoutBinding newBinding = {
        .binding = bindingIndex,
        .descriptorType = descriptorType,
        .descriptorCount = 1,
        .stageFlags = {},
        .pImmutableSamplers = nullptr
    };

    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::clear() {
    bindings.clear();
}

[[nodiscard]] VkDescriptorSetLayout DescriptorLayoutBuilder::buildLayout(VkDevice device, VkShaderStageFlags shaderStages) {
    for (VkDescriptorSetLayoutBinding& binding : bindings) {
        binding.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VkDescriptorSetLayout layout;
    vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout);
    return layout;
}

void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio const> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio poolRatio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize {
            .type = poolRatio.type,
            .descriptorCount = static_cast<uint32_t>(poolRatio.ratio * maxSets)
        });
    }

    VkDescriptorPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    vkCreateDescriptorPool(device, &createInfo, nullptr, &pool);
}

void DescriptorAllocator::clear_descriptors(VkDevice device) {
    vkResetDescriptorPool(device, pool, {});
}

void DescriptorAllocator::destroy_pool(VkDevice device) {
    vkDestroyDescriptorPool(device, pool, nullptr);
}

[[nodiscard]] VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {

    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };
    VkDescriptorSet set;
    vkAllocateDescriptorSets(device, &allocateInfo, &set);
    return set;
}