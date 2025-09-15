#version 450

#define LIGHT_TYPE_POINT       0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_SPOT        2
#define LIGHT_TYPE_AMBIENT     3

//////////////
/// Inputs ///
//////////////

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUvCoord;

///////////////
/// Outputs ///
///////////////

layout(location = 0) out vec4 outColor;

////////////////
/// Uniforms ///
//////////////// 

layout(set = 2, binding = 0) uniform MaterialUBO
{
    vec3 color; // Base color of the material
    float metallic; // Metallic factor of the material
    vec3 emissive_color; // Emissive color of the material
    float roughness; // Roughness factor of the material

    bool use_albedo;
    bool use_normal_map;
    bool use_metallic;
    bool use_emissive;
} material_uniform;


layout(set = 2, binding = 1) uniform sampler2D albedo_texture; // Albedo texture
// layout(set = 2, binding = 2) uniform sampler2D normal_texture; // Normal texture
// layout(set = 2, binding = 3) uniform sampler2D metallic_roughness_texture; // Metallic-Roughness texture
// layout(set = 2, binding = 4) uniform sampler2D emissive_texture; // Emissive texture

struct Light
{
    vec3    light_position;
    float   light_intensity;
    vec3    light_direction;
    uint    light_type;
    vec3    light_color;
    float   spotlight_cutoff;
};

layout(set = 0, binding = 0) buffer Lights
{
    Light lights_buffer[];
};

///////////////////////
/// Light Functions ///
///////////////////////

vec4 ComputeDirectionalLightIntensity(vec3 normal, Light light)
{
    vec4 light_color = vec4(light.light_color, 1.0);
    return light_color * light.light_intensity * max(0.0, dot(normal, -light.light_direction));
}

vec4 ComputePointLightIntensity(vec3 position, vec3 normal, Light light)
{
    vec3 light_dir = light.light_position - position;
    float light_distance = length(light_dir);
    light_dir = normalize(light_dir); // normalize light direction
    vec4 light_color = vec4(light.light_color, 1.0);
    return light_color * light.light_intensity * max(0.0, dot(normal, light_dir)) / (light_distance * light_distance);
}

vec4 ComputeSpotLightIntensity(vec3 position, vec3 normal, Light light)
{
    vec3 light_dir = normalize(light.light_position - position);

    float spot_factor = dot(-light_dir, light.light_direction);

    if (spot_factor > light.spotlight_cutoff)
    {
        vec4 light_color = vec4(light.light_color, 1.0);
        return light_color * light.light_intensity * max(0.0, dot(normal, light_dir)) * (1.0 - (1.0 - spot_factor) / (1.0 - light.spotlight_cutoff));
    }

    return vec4(0.0);
}

vec4 ComputeAmbientLightIntensity(Light light)
{
    vec4 light_color = vec4(light.light_color, 1.0);
    return light_color * light.light_intensity;
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
    else if (light.light_type == LIGHT_TYPE_AMBIENT)
    {
        return ComputeAmbientLightIntensity(light);
    }
}

////////////
/// Main ///
////////////

void main() {
    vec4 final_illumination = vec4(0.0, 0.0, 0.0, 1.0);
    for (uint light_index = 0; light_index < lights_buffer.length(); light_index++)
    {
        final_illumination += ComputeLighting(inPosition, inNormal, lights_buffer[light_index]);
    }
    final_illumination = min(vec4(1.0), final_illumination);

    vec4 diffuse_color = vec4(1.0);
    if (material_uniform.use_albedo)
    {
        diffuse_color = texture(albedo_texture, inUvCoord);
    }
    else
    {
        diffuse_color = vec4(material_uniform.color, 1.0);
    }
    outColor = diffuse_color * final_illumination + vec4(material_uniform.emissive_color, 0.0);
}