#include <Mesh/Mesh.h>

void GPUMesh::cleanup(VmaAllocator allocator) {
    vertexBuffer.cleanup(allocator);
    indexBuffer.cleanup(allocator);
}
