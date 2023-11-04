#pragma once

#include <vector>
#include <unordered_map>
#include "Mesh/RenderMesh.h"
#include "Material.h"
#include "Mesh/Mesh.h"


// Singleton Scene
class Scene
{

    /**
     * The Singleton's constructor should always be private to prevent direct
     * construction calls with the `new` operator.
     */
protected:
    Scene()
    {
    }

    static Scene* scene_;

public:

    /**
     * Singletons should not be cloneable.
     */
    Scene(Scene &other) = delete;
    /**
     * Singletons should not be assignable.
     */
    void operator=(const Scene &) = delete;
    /**
     * This is the static method that controls the access to the singleton
     * instance. On the first run, it creates a singleton object and places it
     * into the static field. On subsequent runs, it returns the client existing
     * object stored in the static field.
     */

    static Scene *GetInstance();

    std::vector<RenderMesh> sceneRenderMeshes;
    std::unordered_map<std::string, Material> sceneMaterialMap;
    std::unordered_map<std::string, Mesh> sceneMeshMap;
    std::vector<GraphicsPipeline> sceneGraphicsPipelines;
};

Scene* Scene::scene_= nullptr;;

/**
 * Static methods should be defined outside the class.
 */
Scene *Scene::GetInstance()
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
    if(scene_==nullptr){
        scene_ = new Scene();
    }
    return scene_;
}