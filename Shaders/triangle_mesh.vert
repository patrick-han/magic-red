#version 450

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in float uv_x;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in float uv_y;
layout (location = 4) in vec3  vTangent;

layout (location = 0) out vec3 fragWorldPos;
layout (location = 1) out vec3 fragWorldNormal;

#include "mesh_push_constants.glsl"

void main() {
    fragWorldPos = vec3(pushConstants.modelMatrix * vec4(vPosition, 1.0));
    fragWorldNormal = mat3(transpose(inverse(pushConstants.modelMatrix))) * vNormal;
    gl_Position = pushConstants.sceneData.projection * pushConstants.sceneData.view * vec4(fragWorldPos, 1.0);
}