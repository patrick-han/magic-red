#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct MeshPushConstants {
    glm::mat4 modelViewProjection;
    glm::mat4 model;

    static VkPushConstantRange range() {
        VkPushConstantRange defaultPushConstantRange = {};
        defaultPushConstantRange.offset = 0;
        defaultPushConstantRange.size = sizeof(MeshPushConstants);
        defaultPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        return defaultPushConstantRange;
    }
};