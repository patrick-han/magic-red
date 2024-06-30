#pragma once
#include <Wrappers/Image.h>
#include <Common/IdTypes.h>
#include <vector>

class GfxDevice;

struct GPUTexture {
    AllocatedImage allocatedImage;
};

struct TextureSize {
    int x{};
    int y{};
    int z{};
};

class TextureCache
{
public:
    TextureCache();
    [[nodiscard]] GPUTextureId add_texture(const GfxDevice& gfxDevice, const void* textureData, const TextureSize& textureSize);
    [[nodiscard]] const GPUTexture& get_texture(GPUTextureId id) const;
    void cleanup(const GfxDevice& gfxDevice);

private:
    void upload_texture(const GfxDevice& gfxDevice, const void* textureData, const TextureSize& textureSize);
    std::vector<GPUTexture> m_gpuTextures;
};
