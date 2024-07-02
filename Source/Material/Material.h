#pragma once

#include <Common/IdTypes.h>

// This point to Ids in a TextureCache
struct Material {
    GPUTextureId diffuseImageId{NULL_GPU_TEXTURE_ID};
    GPUTextureId normalImageId{NULL_GPU_TEXTURE_ID};
    GPUTextureId metallicRoughnessImageId{NULL_GPU_TEXTURE_ID};
    GPUTextureId emissiveImageId{NULL_GPU_TEXTURE_ID};
};
