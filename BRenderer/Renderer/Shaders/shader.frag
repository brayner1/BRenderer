#version 450

#define LIGHT_TYPE_POINT       0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_SPOT        2
#define LIGHT_TYPE_AMBIENT     3

const float PI = 3.14159265359;

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
    vec3 albedo; // Base color of the material
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

layout(set = 1, binding = 1) uniform CameraPos
{
  vec3 camera_position;
} camera_position;

///////////////////////
/// Light Functions ///
///////////////////////

vec3 ComputeDirectionalLightRadiance(Light light)
{
    return light.light_color * light.light_intensity;
}

vec3 ComputePointLightRadiance(vec3 position, Light light)
{
    vec3 light_dir = light.light_position - position;
    float light_distance = length(light_dir);
    light_dir = normalize(light_dir); // normalize light direction
    return light.light_color * light.light_intensity / (light_distance * light_distance);
}

vec3 ComputeSpotLightRadiance(vec3 position, Light light)
{
    vec3 light_dir = normalize(light.light_position - position);

    float spot_factor = dot(-light_dir, light.light_direction);

    if (spot_factor > light.spotlight_cutoff)
    {
        return light.light_color * light.light_intensity * (1.0 - (1.0 - spot_factor) / (1.0 - light.spotlight_cutoff));
    }

    return vec3(0.0);
}

vec3 ComputeAmbientLightRadiance(Light light)
{
    return light.light_color * light.light_intensity;
}

vec3 ComputeLightRadiance(vec3 position, Light light)
{
    if (light.light_type == LIGHT_TYPE_POINT)
    {
        return ComputePointLightRadiance(position, light);
    }
    else if (light.light_type == LIGHT_TYPE_DIRECTIONAL)
    {
        return ComputeDirectionalLightRadiance(light);
    }
    else if (light.light_type == LIGHT_TYPE_SPOT)
    {
        return ComputeSpotLightRadiance(position, light);
    }
    else if (light.light_type == LIGHT_TYPE_AMBIENT)
    {
        return ComputeAmbientLightRadiance(light);
    }
}

vec3 GetLightDirection(Light light, vec3 position)
{
    if (light.light_type == LIGHT_TYPE_POINT || light.light_type == LIGHT_TYPE_SPOT)
    {
        return normalize(light.light_position - position);
    }
    else if (light.light_type == LIGHT_TYPE_DIRECTIONAL)
    {
        return normalize(-light.light_direction);
    }
    return vec3(0.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

////////////
/// Main ///
////////////

void main() {
    vec3 view_dir = normalize(camera_position.camera_position-inPosition);

    vec3 albedo = material_uniform.albedo;
    if (material_uniform.use_albedo)
    {
        albedo = vec3(texture(albedo_texture, inUvCoord));
    }

    vec3 normal = inNormal;
    if (material_uniform.use_normal_map)
    {
        // vec3 tangent = normalize(inTangent);
        // vec3 bitangent = normalize(inBitangent);
        // mat3 TBN = mat3(tangent, bitangent, inNormal);
        // vec3 normal_map = texture(normal_texture, inUvCoord).rgb;
        // normal_map = normal_map * 2.0 - 1.0; // Transform from [0,1] to [-1,1]
        // normal = normalize(TBN * normal_map);
        normal = inNormal;
    }

    float metallic = material_uniform.metallic;
    float roughness = material_uniform.roughness;
    if (material_uniform.use_metallic)
    {
        // metallic = texture(metallic_roughness_texture, inUvCoord).r;
        // roughness = texture(metallic_roughness_texture, inUvCoord).g;
        metallic = material_uniform.metallic;
    }

    vec3 final_illumination = vec3(0.0);

    vec3 F0 = mix (vec3(0.04), albedo, material_uniform.metallic);
    for (uint light_index = 0; light_index < lights_buffer.length(); light_index++)
    {
        vec3 light_dir = GetLightDirection(lights_buffer[light_index], inPosition);
        vec3 halfway_dir = normalize(light_dir + view_dir);

        vec3 light_radiance = ComputeLightRadiance(inPosition, lights_buffer[light_index]);

        float NDF = DistributionGGX(inNormal, halfway_dir, material_uniform.roughness);
        float G = GeometrySmith(inNormal, view_dir, light_dir, material_uniform.roughness);
        vec3 F = FresnelSchlick(max(dot(halfway_dir, view_dir), 0.0), F0);

        vec3 Ks = F;
        vec3 Kd = (vec3(1.0) - Ks) * (1.0 - material_uniform.metallic);

        vec3 num = NDF * G * F;
        float denom = 4.0 * max(dot(inNormal, view_dir), 0.0) * max(dot(inNormal, light_dir), 0.0) + 0.0001;
        vec3 specular = num / denom;

        final_illumination += (Kd * albedo / PI + specular) * light_radiance * max(dot(inNormal, light_dir), 0.0);
    }
    final_illumination += material_uniform.emissive_color;

    vec3 pixel_color = final_illumination / (final_illumination + vec3(1.0));
    pixel_color = pow(pixel_color, vec3(1.0/2.2)); // Gamma correction

    outColor = vec4(pixel_color, 1.0);
}