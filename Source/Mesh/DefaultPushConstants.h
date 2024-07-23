#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <Common/IdTypes.h>

struct DefaultPushConstants {
    glm::mat4 model;
    VkDeviceAddress sceneDataBufferAddress;
    MaterialId materialId;

    static constexpr VkPushConstantRange range() {
        VkPushConstantRange defaultPushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(DefaultPushConstants)
        };
        return defaultPushConstantRange;
    }
};