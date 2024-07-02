#pragma once
#include <Mesh/Mesh.h>
#include <Common/IdTypes.h>

class GfxDevice;
class TextureCache;
class MaterialCache;

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
    CPUModel(const char* fileName, bool isBinary, MaterialCache& _materialCache, TextureCache& _textureCache, const GfxDevice& _gfxDevice);
    CPUMesh m_cpuMesh;
    MaterialId m_materialId;
private:
    MaterialCache& m_materialCache;
    TextureCache& m_textureCache;
    const GfxDevice& m_gfxDevice;
    
};