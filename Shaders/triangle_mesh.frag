#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#include "scene_data.glsl"

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec2 textureCoords;

layout(location = 0) out vec4 outColor;

#include "mesh_push_constants.glsl"


layout (set = 0, binding = 0) uniform sampler linearSampler;
layout (set = 0, binding = 1) uniform texture2D textures[];


void main() {
    // Sample texture(s)
    MaterialData materialData = pushConstants.sceneData.materials.data[pushConstants.materialId];
    vec3 diffuseTexColor = texture(sampler2D(textures[materialData.diffuseTex], linearSampler), textureCoords).rgb;
    vec3 metallicRoughnessColor = texture(sampler2D(textures[materialData.metallicRoughnessTex], linearSampler), textureCoords).rgb;

    // Ambient
    vec3 lightColor = pushConstants.sceneData.pointLights.data[0].color;
    float ambientStrength = 0.1;
    vec3 ambient = diffuseTexColor * ambientStrength * lightColor;

    // Diffuse
    vec3 fragToLightDir = normalize(pushConstants.sceneData.pointLights.data[0].worldSpacePosition - fragWorldPos);
    vec3 norm = normalize(fragWorldNormal);
    float difference = max(dot(fragToLightDir, norm), 0.0);
    vec3 diffuse = diffuseTexColor * difference * lightColor;


    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(pushConstants.sceneData.cameraWorldPosition.xyz - fragWorldPos);
    vec3 reflectDir = reflect(-fragToLightDir, norm);
    float specularDifference = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * specularDifference * lightColor;

    vec3 result = (ambient + diffuse + specular);
    

    // float intensity = 1.0 + sceneDataBuffer.pointLights.data[0].type;
    // intensity = sceneDataBuffer.pointLights.data[0].intensity * intensity;
    // outColor = vec4(difference * fragColor * intensity, 1.0);
    // // outColor = vec4(difference * fragColor, 1.0);

    outColor = vec4(result, 1.0);
}