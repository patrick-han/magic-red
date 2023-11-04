#pragma once

#include <vector>
#include <unordered_map>
#include "Mesh/RenderMesh.h"
#include "Material.h"
#include "Mesh/Mesh.h"

// Singleton Scene
class Scene
{
public:
    static Scene& GetInstance()
    {
        static Scene instance; // Guaranteed to be destroyed.
                               // Instantiated on first use.
        return instance;
    }
private:
    Scene() {}                 // Constructor? (the {} brackets) are needed here.



    // C++ 11
    // =======
    // We can use the better technique of deleting the methods
    // we don't want.
public:
    Scene(Scene const&)          = delete;
    void operator=(Scene const&) = delete;

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status

    // Many RenderMeshes could use the same mesh
    std::vector<RenderMesh> sceneRenderMeshes;
    std::unordered_map<std::string, Mesh> sceneMeshMap;

    // A material is a pipeline
    std::unordered_map<std::string, GraphicsPipeline> sceneMaterialMap;
};