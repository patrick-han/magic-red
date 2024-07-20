#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <Light/DirectionalLight.h>

struct CPUSceneData
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 cameraWorldPosition;
    VkDeviceAddress lightBufferAddress;
    int numPointLights;
    DirectionalLight directionalLight;
    VkDeviceAddress materialBufferAddress;
};

