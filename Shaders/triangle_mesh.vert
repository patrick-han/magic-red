#version 450

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragWorldPos;
layout (location = 2) out vec3 fragWorldNormal;

// Push constants block
layout (push_constant) uniform PushConstants
{
    mat4 modelMatrix;
} pushConstants;

void main() {
    fragWorldPos = vec3(pushConstants.modelMatrix * vec4(vPosition, 1.0));
    fragWorldNormal = mat3(transpose(inverse(pushConstants.modelMatrix))) * vNormal;
    gl_Position = sceneDataBuffer.projection * sceneDataBuffer.view * vec4(fragWorldPos, 1.0);
    fragColor = vColor;
}