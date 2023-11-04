#pragma once
struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 modelViewProjection;

    static VkPushConstantRange range() {
        VkPushConstantRange defaultPushConstantRange = {};
        defaultPushConstantRange.offset = 0;
        defaultPushConstantRange.size = sizeof(MeshPushConstants);
        defaultPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        return defaultPushConstantRange;
    }
};