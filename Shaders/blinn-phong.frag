#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_samplerless_texture_functions : require
#include "scene_data.glsl"

layout(location = 0) in vec2 textureCoords;
layout(location = 0) out vec4 outColor;

#include "mesh_push_constants.glsl"

// TODO subpasses w/ VK_KHR_dynamic_rendering_local_read
layout (set = 0, binding = 0) uniform sampler linearSampler;
layout (set = 0, binding = 1) uniform texture2D textures[];

layout (set = 1, binding = 0) uniform texture2D albedoBuffer;
layout (set = 1, binding = 1) uniform texture2D normalsBuffer;
layout (set = 1, binding = 2) uniform texture2D metallicRoughnessBuffer;
layout (set = 1, binding = 3) uniform texture2D depthBuffer; // For reconstructing world space positions

vec3 calculateDirectionalLightContribution(vec3 diffuseTexColor, vec2 metallicRoughnessColor, vec3 sampledNormal, vec3 fragWorldPos)
{
    vec3 lightColor = vec3(pushConstants.sceneData.directionalLight.power); // TODO: Directional light color?

    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = diffuseTexColor * ambientStrength * lightColor;

    // Diffuse
    vec3 fragToLightDir = normalize(-pushConstants.sceneData.directionalLight.direction);
    vec3 norm = normalize(sampledNormal);
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

vec3 calculatePointLightsContribution(int pointLightIndex, vec3 diffuseTexColor, vec2 metallicRoughnessColor, vec3 sampledNormal, vec3 fragWorldPos)
{
    vec3 lightColor = pushConstants.sceneData.pointLights.data[pointLightIndex].color;

    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = diffuseTexColor * ambientStrength * lightColor;

    // Diffuse
    vec3 fragToLightDir = normalize(pushConstants.sceneData.pointLights.data[pointLightIndex].worldSpacePosition - fragWorldPos);
    vec3 norm = normalize(sampledNormal);
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
        pushConstants.sceneData.pointLights.data[pointLightIndex].linearAttenuation * distance + 
        pushConstants.sceneData.pointLights.data[pointLightIndex].quadraticAttenuation * distance * distance
    );

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    vec3 result = (ambient + diffuse + specular);
    return result;
}


void main() {
    // Sample GBuffer
    vec3 sampledColor = texture(sampler2D(albedoBuffer, linearSampler), textureCoords).rgb;
    vec3 sampledNormal = normalize(texture(sampler2D(normalsBuffer, linearSampler), textureCoords).rgb * 2.0 - 1.0);
    vec2 sampledMetallicRoughness = texture(sampler2D(metallicRoughnessBuffer, linearSampler), textureCoords).rg;
    float sampledDepth = texelFetch(depthBuffer, ivec2(gl_FragCoord.xy), 0).r;
    // x,y are [0, 1] and so is depth-z [0, 1]
    // sampledDepth = sampledDepth * 2.0 - 1.0; // [-1, 1] // In Vulkan, NDC is [0, 1] in z, unlike OpenGL which expects [-1, 1]
    vec4 reconstructedDepth = inverse(pushConstants.sceneData.view) * inverse(pushConstants.sceneData.projection) * vec4(textureCoords * 2.0 - 1.0, sampledDepth, 1.0); // TODO: TEMP
    vec3 fragWorldPos =  reconstructedDepth.xyz / reconstructedDepth.w;

    

    vec3 result = vec3(0.0);

    result += calculateDirectionalLightContribution(sampledColor, sampledMetallicRoughness, sampledNormal, fragWorldPos.xyz);

    for (int i = 0; i < pushConstants.sceneData.numPointLights; i++)
    {
        result += calculatePointLightsContribution(i, sampledColor, sampledMetallicRoughness, sampledNormal, fragWorldPos.xyz);
    }

    outColor = vec4(result, 1.0);
    // outColor = vec4(fragWorldPos.xyz, 1.0);
    // outColor = vec4(sampledDepth, 0.0,0.0,1.0);
}