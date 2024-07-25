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

#include "mesh_push_constants.glsl"


#define DEBUG_VERTEX_COLORS 0

layout (set = 0, binding = 0) uniform sampler linearSampler;
layout (set = 0, binding = 1) uniform texture2D textures[];

vec3 calculateDirectionalLightContribution(vec3 diffuseTexColor, vec3 metallicRoughnessColor)
{
    vec3 lightColor = vec3(pushConstants.sceneData.directionalLight.power); // TODO: Directional light color?

    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = diffuseTexColor * ambientStrength * lightColor;

    // Diffuse
    vec3 fragToLightDir = normalize(-pushConstants.sceneData.directionalLight.direction);
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
    return result;
}

vec3 calculatePointLightsContribution(int pointLightIndex, vec3 diffuseTexColor, vec3 metallicRoughnessColor)
{
    vec3 lightColor = pushConstants.sceneData.pointLights.data[pointLightIndex].color;

    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = diffuseTexColor * ambientStrength * lightColor;

    // Diffuse
    vec3 fragToLightDir = normalize(pushConstants.sceneData.pointLights.data[pointLightIndex].worldSpacePosition - fragWorldPos);
    vec3 norm = normalize(fragWorldNormal);
    float difference = max(dot(fragToLightDir, norm), 0.0);
    vec3 diffuse = diffuseTexColor * difference * lightColor;


    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(pushConstants.sceneData.cameraWorldPosition.xyz - fragWorldPos);
    vec3 reflectDir = reflect(-fragToLightDir, norm);
    float specularDifference = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * specularDifference * lightColor;

    float distance = length(pushConstants.sceneData.pointLights.data[pointLightIndex].worldSpacePosition - fragWorldPos);

    float attenuation = 1.0 / (
        pushConstants.sceneData.pointLights.data[pointLightIndex].constantAttenuation + 
        pushConstants.sceneData.pointLights.data[pointLightIndex].linearAttenuation * distance+ 
        pushConstants.sceneData.pointLights.data[pointLightIndex].quadraticAttenuation * distance * distance
    );

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    vec3 result = (ambient + diffuse + specular);
    return result;
}


void main() {
    // Sample texture(s)
    MaterialData materialData = pushConstants.sceneData.materials.data[pushConstants.materialId];
#if DEBUG_VERTEX_COLORS
    vec3 diffuseTexColor = fragColor.rgb;
#else
    vec3 diffuseTexColor = texture(sampler2D(textures[materialData.diffuseTex], linearSampler), textureCoords).rgb;
#endif
    
    vec3 metallicRoughnessColor = texture(sampler2D(textures[materialData.metallicRoughnessTex], linearSampler), textureCoords).rgb;


    vec3 result = vec3(0.0);

    result += calculateDirectionalLightContribution(diffuseTexColor, metallicRoughnessColor);

    for (int i = 0; i < pushConstants.sceneData.numPointLights; i++)
    {
        result += calculatePointLightsContribution(i, diffuseTexColor, metallicRoughnessColor);
    }

    outColor = vec4(result, 1.0);
    outNormal.rgb = normalize(fragWorldNormal) * 0.5 + 0.5; // Map from [-1, 1] to [0, 1]
}