#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#include "scene_data.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec2 textureCoords;
layout(location = 3) in vec4 fragColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outMetallicRoughness;

#include "mesh_push_constants.glsl"


#define DEBUG_VERTEX_COLORS 0

layout (set = 0, binding = 0) uniform sampler linearSampler;
layout (set = 0, binding = 1) uniform texture2D textures[];

void main() {
    // Sample texture(s)
    MaterialData materialData = pushConstants.sceneData.materials.data[pushConstants.materialId];
#if DEBUG_VERTEX_COLORS
    vec3 diffuseTexColor = fragColor.rgb;
#else
    vec3 diffuseTexColor = texture(sampler2D(textures[materialData.diffuseTex], linearSampler), textureCoords).rgb;
#endif
    
    vec3 metallicRoughnessColor = texture(sampler2D(textures[materialData.metallicRoughnessTex], linearSampler), textureCoords).rgb;

    outColor = vec4(diffuseTexColor, 1.0);
    outNormal.rgb = normalize(fragWorldNormal) * 0.5 + 0.5; // Map from [-1, 1] to [0, 1]
    outMetallicRoughness.rg = metallicRoughnessColor.gb;
}