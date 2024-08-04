#pragma once
#include <Wrappers/Image.h>
#include <Common/IdTypes.h>
#include <vector>
#include <Texture/TextureData.h>
#include <unordered_map>
#include <string>

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

    [[nodiscard]] GPUTextureId add_texture(const GfxDevice& gfxDevice, const TextureLoadingData& texLoadingData, const std::string& textureName);
    [[nodiscard]] const GPUTexture& get_texture(GPUTextureId id) const;
    [[nodiscard]] GPUTextureId get_texture_id(const std::string&e) const;
    [[nodiscard]] uint32_t get_texture_count() const;
    [[nodiscard]] bool is_texture_loaded_already(const std::string&) const;
    [[nodiscard]] GPUTextureId add_render_texture_texture(const GfxDevice& gfxDevice, VkFormat format, VkImageCreateInfo imageCreateInfo);
    [[nodiscard]] const GPUTexture& get_render_texture_texture(GPUTextureId id) const;
    void cleanup(const GfxDevice& gfxDevice);

private:
    void upload_texture(const GfxDevice& gfxDevice, const TextureLoadingData& texLoadingData);
    std::vector<GPUTexture> m_gpuTextures;
    std::vector<GPUTexture> m_gpuRTTextures;
    std::unordered_map<std::string, GPUTextureId> m_texturesLoadedAlready; // std::string (or string_view?) required since doing const char* is comparing different pointers each time
    // TODO: It should really not using std:string as a key, since there is O(N) cost on the string length for both hashing and comparison...
};
