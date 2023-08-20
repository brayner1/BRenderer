#version 450

layout(set = 0, binding = 0) uniform CameraUBO
{
    mat4 projection_view;
} camera_ubo;

layout(set = 1, binding = 0) uniform ModelUBO
{
    mat4 model;
} model_ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 uvCoord;

void main()
{
    gl_Position = camera_ubo.projection_view * model_ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    uvCoord = inUV;
}
