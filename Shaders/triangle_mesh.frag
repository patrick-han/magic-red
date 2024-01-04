#version 450

struct PointLight {
    vec3 worldSpacePosition;
    vec3 color;
};

layout(set = 0, binding = 0) uniform PointLightBuffer {
    layout(offset = 0) PointLight lights[];
} pointLightBuffer;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragWorldNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 fragToLightDir = normalize(pointLightBuffer.lights[0].worldSpacePosition - fragWorldPos);
    vec3 norm = normalize(fragWorldNormal);
    float difference = max(dot(fragToLightDir, norm), 0.0);
    outColor = vec4(difference * fragColor, 1.0);
}