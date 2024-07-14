#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <Common/IdTypes.h>

struct DefaultPushConstants {
    glm::mat4 model;
    VkDeviceAddress sceneDataBufferAddress;
    MaterialId materialId;

    static VkPushConstantRange range() {
        VkPushConstantRange defaultPushConstantRange = {};
        defaultPushConstantRange.offset = 0;
        defaultPushConstantRange.size = sizeof(DefaultPushConstants);
        defaultPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        return defaultPushConstantRange;
    }
};