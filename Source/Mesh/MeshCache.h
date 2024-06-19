#pragma once
#include <Mesh/Mesh.h>
#include <Common/IdTypes.h>

class GfxDevice;

class MeshCache
{
public:
    [[nodiscard]] MeshId add_mesh(const GfxDevice& gfxDevice, const CPUMesh& mesh);
    [[nodiscard]] const GPUMesh& get_mesh(MeshId id) const;
    void cleanup(const GfxDevice& gfxDevice);

private:
    void upload_mesh(const CPUMesh& mesh, VmaAllocator allocator);
    std::vector<GPUMesh> m_meshes;
};