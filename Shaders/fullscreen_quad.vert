#version 450

vec2 vertices[6] = {
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(+1.0, +1.0),
    vec2(-1.0, -1.0),
    vec2(+1.0, +1.0),
    vec2(-1.0, +1.0)
};

vec2 uvs[6] = {
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
};

layout (location = 0) out vec2 textureCoords;

void main() {
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
    textureCoords = uvs[gl_VertexIndex];
}