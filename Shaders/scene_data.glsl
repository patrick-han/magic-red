#ifndef SCENE_DATA_GLSL
#define SCENE_DATA_GLSL

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_buffer_reference : require

#define TYPE_DIRECTIONAL_LIGHT 0
#define TYPE_POINT_LIGHT 1
#define TYPE_SPOT_LIGHT 2

struct PointLight {
    vec3 worldSpacePosition;
    uint type;
    vec3 color;
    float intensity;
};

struct DirectionalLight {
    vec4 direction;
};

layout (buffer_reference, scalar) readonly buffer PointLightsDataBuffer {
    PointLight data[];
};

#include "material.glsl"

layout (buffer_reference, scalar) readonly buffer SceneDataBuffer {
    // camera
    mat4 view;
    mat4 projection;
    // mat4 viewProjection;
    vec3 cameraWorldPosition;
    

    // // ambient
    // vec3 ambientColor;
    // float ambientIntensity;

    // // fog
    // vec3 fogColor;
    // float fogDensity;

    // // CSM data
    // vec4 cascadeFarPlaneZs;
    // mat4 csmLightSpaceTMs[3]; // 3 = NUM_SHADOW_CASCADES
    // uint csmShadowMapId;

    // uint numPointLights;
    PointLightsDataBuffer pointLights;
    int numPointLights;
    DirectionalLight directionalLight;
    // int sunlightIndex; // if -1, there's no sun

    MaterialDataBuffer materials;
};

#endif // SCENE_DATA_GLSL