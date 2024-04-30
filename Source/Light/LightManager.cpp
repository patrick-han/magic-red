#include "LightManager.h"
#include <Common/Log.h>
#include <glm/gtx/transform.hpp>

void LightManager::add_point_light(PointLight&& pointLight)
{
    if(LightManager::GetInstance().pointLightCount < LightManager::MAX_POINT_LIGHTS)
    {
        LightManager::GetInstance().scenePointLights.emplace_back(pointLight);
        LightManager::GetInstance().pointLightCount++;
    }
    else
    {
        MRCERR("Failed to add another point light: Exceeded MAX_POINT_LIGHTS");
    } 
}

uint32_t LightManager::get_point_light_count()
{
    return LightManager::GetInstance().pointLightCount;
}

void LightManager::update_point_lights(int frameNumber)
{
    int lightCircleRadius = 5;
    float lightCircleSpeed = 0.02f;
    LightManager::GetInstance().scenePointLights[0].worldSpacePosition = glm::vec4(lightCircleRadius * glm::cos(lightCircleSpeed * frameNumber), 0.0f, lightCircleRadius * glm::sin(lightCircleSpeed * frameNumber), 0.0f);

    LightManager::GetInstance().scenePointLights[1].worldSpacePosition = glm::vec4(lightCircleRadius * glm::sin(lightCircleSpeed * frameNumber), 0.0f, lightCircleRadius * glm::cos(lightCircleSpeed * frameNumber), 0.0f);
}

PointLight& LightManager::get_point_light_at(int index) const
{
    return LightManager::GetInstance().scenePointLights[index];
}