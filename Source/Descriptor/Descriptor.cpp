#include "Descriptor.h"

/* DescriptorLayoutBuilder */

void DescriptorLayoutBuilder::add_binding(uint32_t bindingIndex, VkDescriptorType descriptorType, uint32_t descriptorCount) {
    VkDescriptorSetLayoutBinding newBinding = {
        .binding = bindingIndex,
        .descriptorType = descriptorType,
        .descriptorCount = descriptorCount,
        .stageFlags = {},
        .pImmutableSamplers = nullptr
    };

    bindings.push_back(newBinding);
}

void DescriptorLayoutBuilder::clear() {
    bindings.clear();
}

[[nodiscard]] VkDescriptorSetLayout DescriptorLayoutBuilder::buildLayout(VkDevice device, VkShaderStageFlags shaderStages, std::span<VkDescriptorBindingFlags const> additionalLayoutBindingFlags, VkDescriptorSetLayoutCreateFlags additionalLayoutCreateFlags) {

    for (VkDescriptorSetLayoutBinding& binding : bindings) {
        binding.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo{ 
        .sType =  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
        .bindingCount = static_cast<uint32_t>(additionalLayoutBindingFlags.size()),
        .pBindingFlags = additionalLayoutBindingFlags.data()
    };


    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &extendedInfo,
        .flags = additionalLayoutCreateFlags,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VkDescriptorSetLayout layout;
    vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout);
    return layout;
}

/* DescriptorAllocator */

void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, std::span<DescriptorTypeCount const> descriptorCounts, VkDescriptorPoolCreateFlags poolFlags) {

    std::vector<VkDescriptorPoolSize> poolSizes;
    for (DescriptorTypeCount descriptorCount : descriptorCounts) {
        poolSizes.push_back(VkDescriptorPoolSize {
            .type = descriptorCount.type,
            .descriptorCount = descriptorCount.count
        });
    }

    VkDescriptorPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = poolFlags,
        .maxSets = maxSets, // How many descriptor sets in total can be allocated from this pool

        // How many descriptors of a certain type will be 
        // allocated NOT in a single descriptor set but in TOTAL from a given pool
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()), // This has nothing to do with descriptor counts, only how large pPoolSizes container is
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