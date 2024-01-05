#include "DynamicRendering.h"

[[nodiscard]] VkRenderingAttachmentInfoKHR rendering_attachment_info(
    VkImageView imageView,
    VkImageLayout imageLayout,
    const VkClearValue *clearValue
    ) {
    
    VkRenderingAttachmentInfoKHR renderingAttachmentInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .pNext = nullptr,
        .imageView = imageView,
        .imageLayout = imageLayout,
        .resolveMode = {},
        .resolveImageView {},
        .resolveImageLayout = {},
        .loadOp = clearValue ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    if(clearValue) {
        renderingAttachmentInfo.clearValue = *clearValue;
    }
    return renderingAttachmentInfo;
}

[[nodiscard]] VkRenderingInfoKHR rendering_info_fullscreen(
    uint32_t colorAttachmentCount,
    VkRenderingAttachmentInfoKHR* pColorAttachments,
    VkRenderingAttachmentInfoKHR* pDepthAttachment
    ) {
    VkRenderingInfoKHR renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .pNext = nullptr,
        .flags = {},
        .renderArea = VkRect2D{ {0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}},
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = colorAttachmentCount,
        .pColorAttachments = pColorAttachments,
        .pDepthAttachment = pDepthAttachment,
        .pStencilAttachment = nullptr,
    };
    return renderingInfo;
}

