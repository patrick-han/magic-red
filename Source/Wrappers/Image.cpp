#include <Wrappers/Image.h>
#include <Wrappers/Buffer.h>
#include <Common/Log.h>
#include <vulkan/vk_enum_string_helper.h> // Doesn't work on linux?
#include <Rendering/GfxDevice.h>

// Transition all mipmap levels and layers by default
VkImageSubresourceRange default_image_subresource_range(VkImageAspectFlags aspectMask)
{
    VkImageSubresourceRange subImage {};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
}

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
    // VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    // imageBarrier.pNext = nullptr;

    // imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    // imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    // imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    // imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    // imageBarrier.oldLayout = currentLayout;
    // imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    // imageBarrier.subresourceRange = default_image_subresource_range(aspectMask);
    // imageBarrier.image = image;

    // VkDependencyInfo depInfo {};
    // depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    // depInfo.pNext = nullptr;

    // depInfo.imageMemoryBarrierCount = 1;
    // depInfo.pImageMemoryBarriers = &imageBarrier;
    // vkCmdPipelineBarrier2(cmd, &depInfo);

    VkImageMemoryBarrier imageBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = default_image_subresource_range(aspectMask)
    };

    vkCmdPipelineBarrier(
        cmd, 
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        {},
        0, nullptr,
        0, nullptr,
        1, &imageBarrier
    );
}

void create_gpu_only_image(AllocatedImage& allocatedImage, VkImageCreateInfo imageCreateInfo, VmaAllocator allocator) {
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkResult res = vmaCreateImage(allocator, &imageCreateInfo, &vmaAllocInfo, &allocatedImage.image, &allocatedImage.allocation, nullptr);
    if (res != VK_SUCCESS) {
        MRCERR(string_VkResult(res));
        throw std::runtime_error("Could not allocate image!");
    }
}

void upload_image(const void *data, AllocatedImage& allocatedImage, VkImageCreateInfo imageCreateInfo, const GfxDevice& gfxDevice) {

    create_gpu_only_image(allocatedImage, imageCreateInfo, gfxDevice.m_vmaAllocator);

    // TODO: HARDCODED FOR RGBA8, 4 bytes per pixel
    size_t bytes_per_channel = 1;
    size_t num_channels = 4;
    size_t data_size = imageCreateInfo.extent.width * imageCreateInfo.extent.height * num_channels * bytes_per_channel;
    AllocatedBuffer imageStagingBuffer;
    upload_buffer(imageStagingBuffer, data_size, data, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, gfxDevice.m_vmaAllocator);

    gfxDevice.immediate_submit([&](VkCommandBuffer cmd) {
        transition_image(cmd, allocatedImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imageCreateInfo.extent;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, imageStagingBuffer.buffer, allocatedImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        transition_image(cmd, allocatedImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    imageStagingBuffer.cleanup(gfxDevice.m_vmaAllocator);
}

void copy_image_to_image(VkCommandBuffer commandBuffer, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) {
    VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(commandBuffer, &blitInfo);
}

[[nodiscard]] VkImageCreateInfo image_create_info(VkFormat format, VkExtent3D extent, VkImageUsageFlags usageFlags, VkImageType imageType) {
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VkImageCreateFlags();
    info.imageType = imageType;

    info.format = format;
    info.extent = extent;

    info.mipLevels = 1;
    info.arrayLayers = 1;

    // For MSAA. we will not be using it by default, so default it to 1 sample per pixel.
    info.samples = VK_SAMPLE_COUNT_1_BIT;

    // Optimal tiling, which means the image is stored on the best gpu format
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usageFlags;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices = nullptr;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return info;
}

[[nodiscard]] VkImageViewCreateInfo imageview_create_info(VkImage image, VkFormat format, VkComponentMapping componentMapping, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VkImageViewCreateFlags();
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.components = componentMapping;
    info.subresourceRange.aspectMask = aspectFlags;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    

    return info;
}