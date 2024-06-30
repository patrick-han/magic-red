#include "TextureCache.h"
#include <Rendering/GfxDevice.h>


TextureCache::TextureCache(const GfxDevice& gfxDevice) : m_gfxDevice(gfxDevice)
{}

[[nodiscard]] GPUTextureId TextureCache::add_texture(const void* textureData, const TextureSize& textureSize) {
   const GPUTextureId textureId = static_cast<uint32_t>(m_gpuTextures.size());
   upload_texture(textureData, textureSize);
   return textureId;
}

void TextureCache::cleanup() {
    for (auto &texture : m_gpuTextures)
    {
        vmaDestroyImage(m_gfxDevice.m_vmaAllocator, texture.allocatedImage.image, texture.allocatedImage.allocation);
        vkDestroyImageView(m_gfxDevice, texture.allocatedImage.imageView, nullptr);
    }
}

void TextureCache::upload_texture(const void* textureData, const TextureSize& textureSize) {

   GPUTexture gpuTexture;

   VkExtent3D imageExtent; 
   imageExtent.width = textureSize.x;
   imageExtent.height = textureSize.y;
   imageExtent.depth = 1;
   gpuTexture.allocatedImage.imageExtent = imageExtent;

   // TODO: hardcoded default format
   VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
   gpuTexture.allocatedImage.imageFormat = format;
   VkImageCreateInfo imageCreateInfo = image_create_info(format, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
   upload_image(static_cast<const void *>(textureData), gpuTexture.allocatedImage, imageCreateInfo, m_gfxDevice);
   VkImageViewCreateInfo imageViewCreateInfo = imageview_create_info(gpuTexture.allocatedImage.image, format, {}, VK_IMAGE_ASPECT_COLOR_BIT);

   VkImageView imageView;
   vkCreateImageView(m_gfxDevice, &imageViewCreateInfo, nullptr, &imageView);
   gpuTexture.allocatedImage.imageView = imageView;
  
   m_gpuTextures.push_back(std::move(gpuTexture)); // TODO: moveable?
}
