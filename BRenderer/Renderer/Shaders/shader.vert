#version 450

layout(binding = 0) uniform UniformBufferObject
{
    mat4 projection_view;
    mat4 model;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main()
{
    gl_Position = ubo.projection_view * model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
