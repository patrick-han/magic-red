#pragma once

#include <Common/IdTypes.h>
#include <Texture/TextureData.h>

// This point to Ids in a TextureCache
struct Material {
    // TODO:
    // "baseColorFactor": [ 1.000, 0.766, 0.336, 1.0 ],
    // "metallicFactor": 1.0,
    // "roughnessFactor": 0.0
    GPUTextureId diffuseTextureId{NULL_GPU_TEXTURE_ID};
    GPUTextureId normalTextureId{NULL_GPU_TEXTURE_ID};
    GPUTextureId metallicRoughnessTextureId{NULL_GPU_TEXTURE_ID};
    GPUTextureId emissiveTextureId{NULL_GPU_TEXTURE_ID};
};