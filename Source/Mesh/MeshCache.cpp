#include "MeshCache.h"
#include <Rendering/GfxDevice.h>


[[nodiscard]] GPUMeshId MeshCache::add_mesh(const GfxDevice& gfxDevice, const CPUMesh& mesh) {
    const GPUMeshId meshId = m_meshes.size();
    upload_mesh(mesh, gfxDevice.m_vmaAllocator);
    return meshId;
}

[[nodiscard]] const GPUMesh& MeshCache::get_mesh(GPUMeshId id) const {
    return m_meshes[id];
}

void MeshCache::cleanup(const GfxDevice& gfxDevice) {
    for (auto &mesh : m_meshes)
    {
        mesh.cleanup(gfxDevice.m_vmaAllocator);
    }
}

void MeshCache::upload_mesh(const CPUMesh& mesh, VmaAllocator allocator) {

    GPUMesh gpuMesh;
    gpuMesh.indexCount = static_cast<uint32_t>(mesh.m_indices.size());

    upload_buffer(gpuMesh.vertexBuffer, mesh.m_vertices.size() * sizeof(Vertex), mesh.m_vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, allocator);

    if (mesh.m_indices.size() > 0) {
        upload_buffer(gpuMesh.indexBuffer, mesh.m_indices.size() * sizeof(uint32_t), mesh.m_indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, allocator);
    }
    gpuMesh.m_materialId = mesh.m_materialId;
    m_meshes.push_back(gpuMesh);
}