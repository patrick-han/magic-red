#include "TextureCache.h"
#include <Rendering/GfxDevice.h>

[[nodiscard]] GPUTextureId TextureCache::add_texture(const GfxDevice& gfxDevice, const TextureLoadingData& texLoadingData, const std::string& textureName) {
    const GPUTextureId textureId = static_cast<uint32_t>(m_gpuTextures.size());
    upload_texture(gfxDevice, texLoadingData);
    if (is_texture_loaded_already(textureName))
    {
        MRCERR("Already loaded this texture without checking is_texture_loaded_already(), did you mean to do this?");
        exit(1);
    }
    m_texturesLoadedAlready.emplace(std::move(textureName), textureId);
    return textureId;
}

[[nodiscard]] const GPUTexture& TextureCache::get_texture(GPUTextureId id) const {
    return m_gpuTextures[id];
}

[[nodiscard]] GPUTextureId TextureCache::get_texture_id(const std::string& textureName) const {
    return m_texturesLoadedAlready.at(textureName);
}

[[nodiscard]] uint32_t TextureCache::get_texture_count() const {
    return static_cast<uint32_t>(m_gpuTextures.size());
}

[[nodiscard]] bool TextureCache::is_texture_loaded_already(const std::string& textureName) const {
    return m_texturesLoadedAlready.count(textureName) > 0 ? true : false;
}


[[nodiscard]] GPUTextureId TextureCache::add_render_texture_texture(const GfxDevice& gfxDevice, VkFormat format, VkImageCreateInfo imageCreateInfo) {
    // const GPUTextureId textureId = static_cast<uint32_t>(m_gpuTextures.size());
    const GPUTextureId textureId = static_cast<uint32_t>(m_gpuRTTextures.size());

    GPUTexture renderTexture;

    VkExtent3D imageExtent; 
    imageExtent.width = WINDOW_WIDTH;
    imageExtent.height = WINDOW_HEIGHT;
    imageExtent.depth = 1;
    renderTexture.allocatedImage.imageExtent = imageExtent;
    renderTexture.allocatedImage.imageFormat = format;

    create_gpu_only_image(renderTexture.allocatedImage, imageCreateInfo, gfxDevice.m_vmaAllocator);
    VkImageViewCreateInfo imageViewCreateInfo = imageview_create_info(renderTexture.allocatedImage.image, format, {}, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(gfxDevice, &imageViewCreateInfo, nullptr, &renderTexture.allocatedImage.imageView);
    

    // m_gpuTextures.push_back(renderTexture);
    m_gpuRTTextures.push_back(renderTexture);
    return textureId;
}

[[nodiscard]] const GPUTexture& TextureCache::get_render_texture_texture(GPUTextureId id) const {
    return m_gpuRTTextures[id];
}


void TextureCache::cleanup(const GfxDevice& gfxDevice) {
    for (auto &texture : m_gpuTextures)
    {
        vkDestroyImageView(gfxDevice, texture.allocatedImage.imageView, nullptr);
        vmaDestroyImage(gfxDevice.m_vmaAllocator, texture.allocatedImage.image, texture.allocatedImage.allocation);
    }

    for (auto &texture : m_gpuRTTextures)
    {
        vkDestroyImageView(gfxDevice, texture.allocatedImage.imageView, nullptr);
        vmaDestroyImage(gfxDevice.m_vmaAllocator, texture.allocatedImage.image, texture.allocatedImage.allocation);
    }
}

void TextureCache::upload_texture(const GfxDevice& gfxDevice, const TextureLoadingData& texLoadingData) {

    GPUTexture gpuTexture;

    VkExtent3D imageExtent; 
    imageExtent.width = texLoadingData.texSize.x;
    imageExtent.height = texLoadingData.texSize.y;
    imageExtent.depth = 1;
    gpuTexture.allocatedImage.imageExtent = imageExtent;

    // TODO: hardcoded default format
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    if (texLoadingData.texSize.ch < 3)
    {
        MRCERR("Invalid texture channel configuration less than 3");
        exit(1);
    }
    gpuTexture.allocatedImage.imageFormat = format;
    VkImageCreateInfo imageCreateInfo = image_create_info(format, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
    upload_image(static_cast<const void *>(texLoadingData.data), texLoadingData.texSize.ch, gpuTexture.allocatedImage, imageCreateInfo, gfxDevice);
    VkImageViewCreateInfo imageViewCreateInfo = imageview_create_info(gpuTexture.allocatedImage.image, format, {}, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(gfxDevice, &imageViewCreateInfo, nullptr, &gpuTexture.allocatedImage.imageView);

    m_gpuTextures.push_back(gpuTexture);
}
