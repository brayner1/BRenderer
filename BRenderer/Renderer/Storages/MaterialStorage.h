#ifndef BRR_MATERIALSTORAGE_H
#define BRR_MATERIALSTORAGE_H

#include <Core/thirdpartiesInc.h>
#include <Renderer/RenderDefs.h>
#include <Renderer/RenderingResourceIDs.h>
#include <Renderer/RenderThread.h>
#include <Renderer/Shader.h>
#include <Renderer/GpuResources/GpuResourcesHandles.h>

#include "BaseStorage.h"

namespace brr::render
{
    struct MaterialProperties
    {
        glm::vec3 color = glm::vec3(1.0f);
        float metallic = 0.0f;
        glm::vec3 emissive_color = glm::vec3(0.0f);
        float roughness = 0.5f;

        TextureID diffuse_texture {};
        TextureID normal_texture{};
        TextureID metallic_roughness_texture{};
        TextureID emissive_texture{};
    };

    struct Material
    {
        MaterialProperties properties;

        std::array<DescriptorSetHandle, FRAME_LAG> descriptor_sets{};

        BufferHandle uniform_buffer_handle {};

        ShaderID shader_id;
    };

    MaterialProperties BuildMaterialProperties(brr::vis::MaterialData material_data);

    class MaterialStorage : public BaseStorage<Material, MaterialID>
    {
      public:
        MaterialStorage();
        ~MaterialStorage() = default;

        void InitializeDefaults();

        void DestroyDefaults();

        // Shader

        ShaderID CreateShader(const std::string& shader_name,
                              const std::string& shader_folder_path,
                              bool make_default = false);

        void DestroyShader(ShaderID shader_handle);

        Shader* GetShader(ShaderID shader_handle) const;

        ShaderID GetDefaultShaderID() const { return m_default_shader; }

        // Material

        void InitMaterial(MaterialID material_id,
                          MaterialProperties material_properties,
                          ShaderID shader_id = ShaderID());

        void DestroyMaterial(MaterialID material_id);

        Material* GetMaterial(MaterialID material_id);

        void UpdateMaterialProperties(MaterialID material_id, const MaterialProperties& properties);

        // Frame Update

        void FrameUpdatePendingDescriptors();

    private:

        std::array<std::vector<MaterialID>, FRAME_LAG> m_frames_updates;

        ResourceAllocator<Shader> m_shader_storage;
        ShaderID m_default_shader;
        TextureID m_null_texture;
    };
}

#endif