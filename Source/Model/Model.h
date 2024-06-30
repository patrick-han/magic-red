#pragma once
#include <Mesh/Mesh.h>
#include <Common/IdTypes.h>

class GfxDevice;
class TextureCache;

struct NodeLoadingData {
    std::vector<Vertex>& vertices;
    std::vector<uint32_t>& indices;
    size_t indexPos{};
    size_t vertexPos{};
    // Each node has a single mesh?
    // TODO: Technically a mesh could have multiple materials (each "primitive" could have a different material)
    // but lets keep it simple for now
    const void *diffuseTextureData;
    int diffuseTextureSizeX;
    int diffuseTextureSizeY;
    int diffuseTextureSizeZ;
};

struct CPUModel {
    CPUModel(const char* fileName, bool isBinary, GfxDevice& _gfxDevice, TextureCache& _textureCache);
    CPUMesh m_cpuMesh;
    GfxDevice& m_gfxDevice;
    TextureCache& m_textureCache;
};