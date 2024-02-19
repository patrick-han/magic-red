#pragma once

#include <vector>
#include <unordered_map>
#include <Mesh/RenderObject.h>
#include <Pipeline/MaterialFunctions.h>
#include <Mesh/Mesh.h>
#include <Light/PointLight.h>

// Singleton Scene
class Scene
{
public:
    static Scene& GetInstance()
    {
        static Scene instance; // Guaranteed to be destroyed.
                               // Instantiated on first use (also known as lazy initialization).
        return instance;
    }
private:
    Scene() {}                 // Constructor.
                               // Obviously, we don't want users creating another SINGLEton.



    // C++ 11
    // =======
    // We can use this technique of deleting the methods
    // we don't want.
public:
    Scene(Scene const&)          = delete; // Copy constructor
    void operator=(Scene const&) = delete; // Copy assignment

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status

    // Many RenderObjects could use the same mesh
    std::vector<RenderObject> sceneRenderObjects;
    std::unordered_map<std::string, Mesh> sceneMeshMap;

    // A material is a pipeline
    std::unordered_map<std::string, GraphicsPipeline> sceneMaterialMap;

    // Lights
    std::vector<PointLight> scenePointLights;
};