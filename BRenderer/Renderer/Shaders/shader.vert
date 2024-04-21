#version 450

layout(set = 1, binding = 0) uniform CameraUBO
{
    mat4 projection_view;
} camera_ubo;

layout(set = 3, binding = 0) uniform ModelUBO
{
    mat4 model;
} model_ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float u_texcoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in float v_texcoord;
layout(location = 4) in vec3 tangent;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBitangent;
layout(location = 4) out vec2 uvCoord;

void main()
{
    gl_Position = camera_ubo.projection_view * model_ubo.model * vec4(inPosition, 1.0);
    outPosition = vec3(model_ubo.model * vec4(inPosition, 1.0));
    outNormal = vec3(model_ubo.model * vec4(normal, 0.0));
    outTangent = vec3(model_ubo.model * vec4(tangent, 0.0));
    outBitangent = cross(outNormal, outTangent);
    uvCoord = vec2 (u_texcoord, v_texcoord);
}
