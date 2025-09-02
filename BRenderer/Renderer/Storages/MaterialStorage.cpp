#include "MaterialStorage.h"

#include <Geometry/Geometry.h>
#include <Renderer/Shader.h>
#include <Renderer/GpuResources/Descriptors.h>

#include "RenderStorageGlobals.h"
#include "TextureStorage.h"

using namespace brr::render;

MaterialProperties brr::render::BuildMaterialProperties(brr::vis::MaterialData material_data)
{
    TextureID diffuse_texture = material_data.diffuse_texture ? material_data.diffuse_texture->GetTextureID() : TextureID();
    TextureID normal_texture = material_data.normal_texture ? material_data.normal_texture->GetTextureID() : TextureID();
    TextureID metallic_roughness_texture = material_data.metallic_roughness_texture ? material_data.metallic_roughness_texture->GetTextureID() : TextureID();
    TextureID emissive_texture = material_data.emissive_texture ? material_data.emissive_texture->GetTextureID() : TextureID();
    return {
        material_data.color,
        material_data.metallic,
        material_data.emissive_color,
        material_data.roughness,
        diffuse_texture,
        normal_texture,
        metallic_roughness_texture,
        emissive_texture
    };
}

struct MaterialUniformData
{
    glm::vec3 color;
    float metallic;
    glm::vec3 emissive_color;
    float roughness;

    bool use_albedo;
    bool use_normal_map;
    bool use_metallic_roughness;
    bool use_emissive;
};

MaterialStorage::MaterialStorage() : BaseStorage()
{}

void MaterialStorage::InitializeDefaults()
{
    // Create default shader
    m_default_shader = CreateShader("DefaultShader", "Engine/Shaders", true);

    // Create null texture (1x1 white pixel)
    m_null_texture = RenderStorageGlobals::texture_storage.AllocateTexture();
    uint32_t texture_color = 0xFFFFFFFF; // White color
    RenderStorageGlobals::texture_storage.InitTexture(m_null_texture, &texture_color, 1, 1, DataFormat::R8G8B8A8_SRGB);
}

void MaterialStorage::DestroyDefaults()
{
    // Destroy null texture
    RenderStorageGlobals::texture_storage.DestroyTexture(m_null_texture);
    m_null_texture = TextureID();

    // Destroy default shader
    DestroyShader(m_default_shader);
    m_default_shader = ShaderID();
}

ShaderID MaterialStorage::CreateShader(const std::string& shader_name,
                                       const std::string& shader_folder_path,
                                       bool make_default)
{
    // Sets should be organized from less frequently updated to more frequently updated.
    ShaderBuilder shader_builder;
    shader_builder
        .SetVertexShaderFile(shader_folder_path + "/vert.spv")
        .SetFragmentShaderFile(shader_folder_path + "/frag.spv")
        .AddVertexInputBindingDescription(0, sizeof(Vertex3))
        .AddVertexAttributeDescription(0, 0, DataFormat::R32G32B32_Float, offsetof(Vertex3, pos))
        .AddVertexAttributeDescription(0, 1, DataFormat::R32_Float, offsetof(Vertex3, u))
        .AddVertexAttributeDescription(0, 2, DataFormat::R32G32B32_Float, offsetof(Vertex3, normal))
        .AddVertexAttributeDescription(0, 3, DataFormat::R32_Float, offsetof(Vertex3, v))
        .AddVertexAttributeDescription(0, 4, DataFormat::R32G32B32_Float, offsetof(Vertex3, tangent))
        .AddSet() // Set 0 -> Binding 0: Lights array
        .AddSetBinding(DescriptorType::StorageBuffer, FragmentShader)
        .AddSet() // Set 1 -> Binding 0: Camera view and projection
        .AddSetBinding(DescriptorType::UniformBuffer, VertexShader)
        .AddSet() // Set 2 -> Binding 0: Material transform.
        .AddSetBinding(DescriptorType::UniformBuffer, FragmentShader)
        .AddSetBinding(DescriptorType::CombinedImageSampler, FragmentShader)
        .AddSet() // Set 3 -> Binding 0: Model Uniform
        .AddSetBinding(DescriptorType::UniformBuffer, VertexShader);

    Shader* shader_ptr;
    ResourceHandle shader_handle = m_shader_storage.CreateResource(&shader_ptr, shader_builder.BuildShader());

    if (make_default)
    {
        m_default_shader = shader_handle;
    }
    
    return shader_handle;
}

void MaterialStorage::DestroyShader(ShaderID shader_handle)
{
    Shader* shader = m_shader_storage.GetResource(shader_handle);
    if (shader)
    {
        shader->DestroyShaderModules();
        m_shader_storage.DestroyResource(shader_handle);
    }
}

Shader* MaterialStorage::GetShader(ShaderID shader_handle) const
{
    return m_shader_storage.GetResource(shader_handle);
}

void MaterialStorage::InitMaterial(MaterialID material_id,
                                   MaterialProperties material_properties,
                                   ShaderID shader_id)
{
    Material* material = InitResource(material_id, Material());

    if (material == nullptr)
    {
        BRR_LogError("Trying to initialize Material (ID: {}) that does not exist.", static_cast<uint64_t>(material_id));
        return;
    }

    material->properties = material_properties;

    BRR_LogInfo("Initializing Material (ID: {}).", static_cast<uint64_t>(material_id));

    if (!shader_id.IsValid())
    {
        shader_id = m_default_shader;
        BRR_LogInfo("Using default shader for Material (ID: {}).", static_cast<uint64_t>(material_id));
    }

    Shader* shader = m_shader_storage.GetResource(shader_id);
    if (shader == nullptr)
    {
        BRR_LogError("Trying to initialize Material (ID: {}) with a non-existing Shader (ID: {}).", static_cast<uint64_t>(material_id), static_cast<uint64_t>(shader_id));
        return;
    }

    DescriptorLayout descriptor_layout = shader->GetDescriptorSetLayouts()[2]; // Set 2 is the Material set

    VulkanRenderDevice* render_device = VKRD::GetSingleton();
    if (!render_device)
    {
        BRR_LogError("VulkanRenderDevice is not initialized.");
        return;
    }

    material->uniform_buffer_handle = render_device->CreateBuffer(sizeof(MaterialUniformData),
                                                                  BufferUsage::UniformBuffer | BufferUsage::TransferDst,
                                                                  MemoryUsage::AUTO_PREFER_DEVICE);

    BRR_LogInfo("Initialized Material (ID: {}) uniform buffer.", static_cast<uint64_t>(material_id));
    MaterialUniformData material_uniform_data = {};
    material_uniform_data.color = material_properties.color;
    material_uniform_data.metallic = material_properties.metallic;
    material_uniform_data.emissive_color = material_properties.emissive_color;
    material_uniform_data.roughness = material_properties.roughness;
    material_uniform_data.use_albedo = material_properties.diffuse_texture.IsValid();
    material_uniform_data.use_normal_map = material_properties.normal_texture.IsValid();
    material_uniform_data.use_metallic_roughness = material_properties.metallic_roughness_texture.IsValid();
    material_uniform_data.use_emissive = material_properties.emissive_texture.IsValid();

    render_device->UploadBufferData(material->uniform_buffer_handle, &material_uniform_data, sizeof(MaterialUniformData), 0);

    BRR_LogInfo("Uploaded Material (ID: {}) uniform buffer data.", static_cast<uint64_t>(material_id));

    std::vector<DescriptorSetHandle> descriptors_handles = render_device->DescriptorSet_Allocate(
        descriptor_layout.m_layout_handle, FRAME_LAG);
    std::ranges::copy(descriptors_handles, material->descriptor_sets.begin());
    BRR_LogInfo("Allocated {} descriptor sets for Material (ID: {}).", FRAME_LAG, static_cast<uint64_t>(material_id));

    Texture2DHandle diffuse_texture_handle;
    TextureID texture_id = material_properties.diffuse_texture.IsValid() ? material_properties.diffuse_texture : m_null_texture;
    TextureStorage::Texture* diffuse_texture = RenderStorageGlobals::texture_storage.GetTexture(texture_id);
    if (diffuse_texture != nullptr)
    {
        diffuse_texture_handle = diffuse_texture->texture_2d_handle;
    }
    else
    {
        diffuse_texture = RenderStorageGlobals::texture_storage.GetTexture(m_null_texture);
        assert(diffuse_texture != nullptr && "MaterialStorage::m_null_texture must represent a valid 1x1 texture.");
        diffuse_texture_handle = diffuse_texture->texture_2d_handle;
    }

    BRR_LogInfo("Binding resources to Material (ID: {}) descriptor sets.", static_cast<uint64_t>(material_id));
    for (uint32_t frame_idx = 0; frame_idx < FRAME_LAG; frame_idx++)
    {
        auto setBuilder = DescriptorSetUpdater(descriptor_layout);

        // Build the material descriptor set based on the properties from shader
        setBuilder.BindBuffer(0, material->uniform_buffer_handle, sizeof(MaterialUniformData)); // Updating the buffer with material uniform data
        setBuilder.BindImage(1, diffuse_texture_handle);

        setBuilder.UpdateDescriptorSet(material->descriptor_sets[frame_idx]);
    }
}

void MaterialStorage::DestroyMaterial(MaterialID material_id)
{
    Material* material = GetMaterial(material_id);
    if (!material)
    {
        BRR_LogError("Trying to destroy Material (ID: {}) that does not exist.", static_cast<uint64_t>(material_id));
        return;
    }

    VulkanRenderDevice* render_device = VKRD::GetSingleton();
    if (!render_device)
    {
        BRR_LogError("VulkanRenderDevice is not initialized.");
        return;
    }

    render_device->DestroyBuffer(material->uniform_buffer_handle);
    for (auto& descriptor_set : material->descriptor_sets)
    {
        render_device->DescriptorSet_Destroy(descriptor_set);
    }
    DestroyResource(material_id);
}

Material* MaterialStorage::GetMaterial(MaterialID material_id)
{
    return GetResource(material_id);
}

void MaterialStorage::UpdateMaterialProperties(MaterialID material_id,
                                               const MaterialProperties& properties)
{
}
