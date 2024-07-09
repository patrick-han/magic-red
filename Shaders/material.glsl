#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#extension GL_EXT_buffer_reference : require

// Based on the standard set of textures provided by the Damaged Helmet gltf: https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/DamagedHelmet/glTF, excluding AO because I want to do it in real time
struct MaterialData {
    vec4 baseColor;
    uint diffuseTex;
    uint normalTex;
    uint metallicRoughnessTex;
    uint emissiveTex;
};

layout (buffer_reference, std430) readonly buffer MaterialDataBuffer {
    MaterialData data[];
} materialDataBuffer;

#endif // MATERIAL_GLSL