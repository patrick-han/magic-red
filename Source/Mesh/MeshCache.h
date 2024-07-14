#pragma once
#include <Mesh/Mesh.h>
#include <Common/IdTypes.h>

class GfxDevice;

class MeshCache
{
public:
    MeshCache() = default;
    ~MeshCache() = default;
    MeshCache(const MeshCache&) = delete;
    MeshCache& operator=(const MeshCache&) = delete;
    MeshCache(MeshCache&&) = delete;
    MeshCache& operator=(MeshCache&&) = delete;

    [[nodiscard]] GPUMeshId add_mesh(const GfxDevice& gfxDevice, const CPUMesh& mesh);
    [[nodiscard]] const GPUMesh& get_mesh(GPUMeshId id) const;
    void cleanup(const GfxDevice& gfxDevice);

private:
    void upload_mesh(const CPUMesh& mesh, VmaAllocator allocator);
    std::vector<GPUMesh> m_meshes;
};