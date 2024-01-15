#version 450

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 tangent;
layout(location = 2) in vec3 bitangent;
layout(location = 3) in vec2 uvCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

void main() {
    vec3 positiveNormal = (normal + vec3(1.0)) / 2.0;
    outColor = texture(texSampler, uvCoord) * vec4(positiveNormal, 1.0);
}