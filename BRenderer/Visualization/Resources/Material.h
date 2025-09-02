#ifndef BRR_MATERIAL_H
#define BRR_MATERIAL_H

//#include <Core/thirdpartiesInc.h>
#include <Core/Assets/Asset.h>
#include <Core/Ref.h>
#include <glm/vec3.hpp>
#include <Visualization/Resources/Image.h>


namespace brr::render
{
    struct MaterialProperties;
}

namespace brr::vis
{
    struct MaterialData
    {
        glm::vec3 color          = glm::vec3(1.0f);
        float metallic           = 0.0f;
        glm::vec3 emissive_color = glm::vec3(0.0f);
        float roughness          = 0.5f;

        Ref<Image> diffuse_texture{};
        Ref<Image> normal_texture{};
        Ref<Image> metallic_roughness_texture{};
        Ref<Image> emissive_texture{};
    };

    class Material : public Asset
    {
    public:
        Material();

        Material(MaterialData material_data);

        Material(const Material& other);

        Material(Material&& other) noexcept;

        Material& operator=(const Material& other);

        Material& operator=(Material&& other) noexcept;

        ~Material() override;

        render::MaterialID GetMaterialID() const { return m_material_id; }

        // Colors and factors

        const glm::vec3& GetDiffuseColor() const { return m_material_data.color; }
        const glm::vec3& GetEmissiveColor() const { return m_material_data.emissive_color; }
        float GetMetallic() const { return m_material_data.metallic; }
        float GetRoughness() const { return m_material_data.roughness; }

        void SetDiffuseColor(const glm::vec3& color);
        void SetEmissiveColor(const glm::vec3& color);
        void SetMetallic(float metallic);
        void SetRoughness(float roughness);

        // Textures

        Ref<Image> GetDiffuseTexture() const { return m_material_data.diffuse_texture; }
        Ref<Image> GetNormalTexture() const { return m_material_data.normal_texture; }
        Ref<Image> GetMetallicRoughnessTexture() const { return m_material_data.metallic_roughness_texture; }
        Ref<Image> GetEmissiveTexture() const { return m_material_data.emissive_texture; }

        void SetDiffuseTexture(Ref<Image> texture);
        void SetNormalTexture(Ref<Image> texture);
        void SetMetallicRoughnessTexture(Ref<Image> texture);
        void SetEmissiveTexture(Ref<Image> texture);

    private:

        static render::MaterialProperties BuildMaterialProperties(MaterialData material_data);

        MaterialData m_material_data;
        render::MaterialID m_material_id;
    };
}

#endif // BRR_MATERIAL_H
