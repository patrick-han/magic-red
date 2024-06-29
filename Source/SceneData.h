#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct CPUSceneData
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 cameraWorldPosition;
    // uint32_t numPointLights;
    VkDeviceAddress lightBufferAddress;
    // static void build_descriptor_set_layout();
};

