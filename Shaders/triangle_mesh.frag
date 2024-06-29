#version 450

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragWorldNormal;

layout(location = 0) out vec4 outColor;

void main() {
    // Ambient
    vec3 lightColor = sceneDataBuffer.pointLights.data[0].color;
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 fragToLightDir = normalize(sceneDataBuffer.pointLights.data[0].worldSpacePosition - fragWorldPos);
    vec3 norm = normalize(fragWorldNormal);
    float difference = max(dot(fragToLightDir, norm), 0.0);
    vec3 diffuse = difference * lightColor;


    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(sceneDataBuffer.cameraWorldPosition.xyz - fragWorldPos);
    vec3 reflectDir = reflect(-fragToLightDir, norm);
    float specularDifference = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * specularDifference * lightColor;


    vec3 result = (ambient + diffuse + specular) * fragColor;
    

    // float intensity = 1.0 + sceneDataBuffer.pointLights.data[0].type;
    // intensity = sceneDataBuffer.pointLights.data[0].intensity * intensity;
    // outColor = vec4(difference * fragColor * intensity, 1.0);
    // // outColor = vec4(difference * fragColor, 1.0);

    outColor = vec4(result, 1.0);
}