#version 450

#define LIGHT_TYPE_POINT       0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_SPOT        2

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

vec4 ComputeDirectionalLightIntensity(vec3 normal, Light light)
{
    return light.light_color * light.light_intensity * max(0.0, dot(normal, -light.light_direction));
}

vec4 ComputePointLightIntensity(vec3 position, vec3 normal, Light light)
{
    vec3 light_dir = light.light_position - position;
    float light_distance = length(light_dir);
    light_dir = normalize(light_dir); // normalize light direction
    return light.light_color * light.light_intensity * max(0.0, dot(normal, light_dir)) / (light_distance * light_distance);
}

vec4 ComputeSpotLightIntensity(vec3 position, vec3 normal, Light light)
{
    vec3 light_dir = normalize(light.light_position - position);

    float spot_factor = dot(-light_dir, light.light_direction);
    float cuttof_cos = light.light_color.w;

    if (spot_factor > cuttof_cos)
    {
        vec4 light_color = vec4(light.light_color.xyz, 1.0);
        return light_color * light.light_intensity * max(0.0, dot(normal, light_dir)) * (1.0 - (1.0 - spot_factor) / (1.0 - cuttof_cos));
    }

    return vec4(0.0);
}

vec4 ComputeLighting(vec3 position, vec3 normal, Light light)
{
    if (light.light_type == LIGHT_TYPE_POINT)
    {
        return ComputePointLightIntensity(position, normal, light);
    }
    else if (light.light_type == LIGHT_TYPE_DIRECTIONAL)
    {
        return ComputeDirectionalLightIntensity(normal, light);
    }
    else if (light.light_type == LIGHT_TYPE_SPOT)
    {
        return ComputeSpotLightIntensity(position, normal, light);
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