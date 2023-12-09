#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 uvCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, uvCoord);
    //outColor = vec4(fragColor, 0.0);
}