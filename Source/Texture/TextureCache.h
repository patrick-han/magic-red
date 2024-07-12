#pragma once
#include <Wrappers/Image.h>
#include <Common/IdTypes.h>
#include <vector>
#include <Texture/TextureData.h>

class GfxDevice;

struct GPUTexture {
    AllocatedImage allocatedImage;
};

class TextureCache
{
public:
    TextureCache() = default;
    ~TextureCache() = default;
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;
    TextureCache(TextureCache&&) = delete;
    TextureCache& operator=(TextureCache&&) = delete;

    [[nodiscard]] GPUTextureId add_texture(const GfxDevice& gfxDevice, const TextureLoadingData& texLoadingData);
    [[nodiscard]] const GPUTexture& get_texture(GPUTextureId id) const;
    void cleanup(const GfxDevice& gfxDevice);

private:
    void upload_texture(const GfxDevice& gfxDevice, const TextureLoadingData& texLoadingData);
    std::vector<GPUTexture> m_gpuTextures;
};
