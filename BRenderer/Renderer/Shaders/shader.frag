#version 450

#define POINT_LIGHT_TYPE 0

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUvCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

struct Light
{
    vec3    light_position;
    float   light_intensity;
    vec3    light_direction;
    uint    light_type;
    vec4    light_color;
};

layout(set = 0, binding = 1) buffer Lights
{
    Light lights_buffer[];
};

vec4 ComputePointLightIntensity(vec3 position, vec3 normal, Light light)
{
    vec3 light_dir = light.light_position - position;
    float light_distance = length(light_dir);
    light_dir = light_dir / light_distance; // normalize light direction
    return light.light_color * light.light_intensity * max(0.0, dot(normal, light_dir)) / (light_distance * light_distance);
}

vec4 ComputeLighting(vec3 position, vec3 normal, Light light)
{
    if (light.light_type == POINT_LIGHT_TYPE)
    {
        return ComputePointLightIntensity(position, normal, light);
    }
}

void main() {
    vec4 final_illumination = vec4(0.0, 0.0, 0.0, 1.0);
    for (uint light_index = 0; light_index < lights_buffer.length(); light_index++)
    {
        final_illumination += ComputeLighting(inPosition, inNormal, lights_buffer[light_index]);
    }
    final_illumination = min(vec4(1.0), final_illumination);
    outColor = texture(texSampler, inUvCoord) * final_illumination;
}