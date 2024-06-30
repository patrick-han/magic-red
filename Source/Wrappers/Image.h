#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

class GfxDevice;


VkImageSubresourceRange default_image_subresource_range(VkImageAspectFlags aspectMask);

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

/* Upload an image to the GPU */
void create_gpu_only_image(AllocatedImage& allocatedImage, VkImageCreateInfo imageCreateInfo, VmaAllocator allocator);

void upload_image(const void *data, AllocatedImage& allocatedImage, VkImageCreateInfo imageCreateInfo, const GfxDevice& gfxDevice);

/* Copy one image to another */
void copy_image_to_image(VkCommandBuffer commandBuffer, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);

/* Generate a sensible default image create info */
[[nodiscard]] VkImageCreateInfo image_create_info(VkFormat format, VkExtent3D extent, VkImageUsageFlags usageFlags, VkImageType imageType);

/* Generate a sensible default image view create info */
[[nodiscard]] VkImageViewCreateInfo imageview_create_info(VkImage image, VkFormat format, VkComponentMapping componentMapping, VkImageAspectFlags aspectFlags);