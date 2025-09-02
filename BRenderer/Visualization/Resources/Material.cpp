#include "Material.h"

#include <Renderer/RenderThread.h>

using namespace brr;
using namespace brr::vis;

Material::Material()
{
    m_material_id = render::RenderThread::ResourceCmd_CreateMaterial(m_material_data);
}

Material::Material(MaterialData material_data)
{
    m_material_data = std::move(material_data);
    m_material_id   = render::RenderThread::ResourceCmd_CreateMaterial(m_material_data);
}

Material::Material(const Material& other)
{
    *this = other;
}

Material::Material(Material&& other) noexcept
{
    *this = std::move(other);
}

Material& Material::operator=(const Material& other)
{
    m_material_data = other.m_material_data;

    m_material_id = render::RenderThread::ResourceCmd_CreateMaterial(m_material_data);
    return *this;
}

Material& Material::operator=(Material&& other) noexcept
{
    m_material_data = std::move(other.m_material_data);
    m_material_id   = other.m_material_id;

    other.m_material_id = render::MaterialID();
    return *this;
}

Material::~Material()
{
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_DestroyMaterial(m_material_id);
    }
}

void Material::SetDiffuseColor(const glm::vec3& color)
{
    glm::vec3 clamped_color = glm::clamp(color, 0.0f, 1.0f);
    m_material_data.color   = clamped_color;
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_UpdateMaterialProperties(m_material_id, m_material_data);
    }
}

void Material::SetEmissiveColor(const glm::vec3& color)
{
    glm::vec3 clamped_emissive     = glm::clamp(color, 0.0f, 1.0f);
    m_material_data.emissive_color = clamped_emissive;
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_UpdateMaterialProperties(m_material_id, m_material_data);
    }
}

void Material::SetMetallic(float metallic)
{
    float clamped_metallic   = glm::clamp(metallic, 0.0f, 1.0f);
    m_material_data.metallic = clamped_metallic;
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_UpdateMaterialProperties(m_material_id, m_material_data);
    }
}

void Material::SetRoughness(float roughness)
{
    float clamped_roughness   = glm::clamp(roughness, 0.0f, 1.0f);
    m_material_data.roughness = clamped_roughness;
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_UpdateMaterialProperties(m_material_id, m_material_data);
    }
}

void Material::SetDiffuseTexture(Ref<Image> texture)
{
    m_material_data.diffuse_texture = texture;
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_UpdateMaterialProperties(m_material_id, m_material_data);
    }
}

void Material::SetNormalTexture(Ref<Image> texture)
{
    m_material_data.normal_texture = texture;
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_UpdateMaterialProperties(m_material_id, m_material_data);
    }
}

void Material::SetMetallicRoughnessTexture(Ref<Image> texture)
{
    m_material_data.metallic_roughness_texture = texture;
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_UpdateMaterialProperties(m_material_id, m_material_data);
    }
}

void Material::SetEmissiveTexture(Ref<Image> texture)
{
    m_material_data.emissive_texture = texture;
    if (m_material_id.IsValid())
    {
        render::RenderThread::ResourceCmd_UpdateMaterialProperties(m_material_id, m_material_data);
    }
}
