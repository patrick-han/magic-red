#pragma once

#include <vector>
#include <Light/PointLight.h>

class LightManager
{
public:
    static LightManager& GetInstance()
    {
        static LightManager instance; // Guaranteed to be destroyed.
                               // Instantiated on first use (also known as lazy initialization).
        return instance;
    }
private:
    LightManager() {}                 // Constructor.
                               // Obviously, we don't want users creating another SINGLEton.

    std::vector<PointLight> scenePointLights;

    uint32_t pointLightCount = 0;

    // C++ 11
    // =======
    // We can use this technique of deleting the methods
    // we don't want.
public:
    LightManager(LightManager const&)          = delete; // Copy constructor
    void operator=(LightManager const&) = delete; // Copy assignment

    // Note: Scott Meyers mentions in his Effective Modern
    //       C++ book, that deleted functions should generally
    //       be public as it results in better error messages
    //       due to the compilers behavior to check accessibility
    //       before deleted status

    static const uint32_t MAX_POINT_LIGHTS = 64;
    void add_point_light(PointLight&& pointLight);
    uint32_t get_point_light_count();
    void update_point_lights(int frameNumber);
    PointLight& get_point_light_at(int index) const;
};