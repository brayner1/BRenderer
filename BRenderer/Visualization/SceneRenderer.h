#ifndef BRR_SCENERENDERER_H
#define BRR_SCENERENDERER_H
#include <Core/Storage/ContiguousPool.h>
#include <Geometry/Geometry.h>
#include <Renderer/Descriptors.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/RenderDefs.h>
#include <Scene/Scene.h>
#include <Scene/Components/Mesh3DComponent.h>
#include <Scene/Components/NodeComponent.h>
#include <Visualization/Image.h>

namespace brr::vis
{
    enum class SurfaceId : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class LightId : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    enum class ViewportId : uint32_t
    {
        NULL_ID = static_cast<uint32_t>(-1)
    };

    class SceneRenderer
    {
    public:

        SceneRenderer(Scene* scene);

        ~SceneRenderer();

        //-------------------------//
        //--- Surface Functions ---//
        //-------------------------//

        SurfaceId CreateNewSurface(Mesh3DComponent::SurfaceData& surface, const Entity& owner_entity);
        void RemoveSurface(SurfaceId surface_id);

        //------------------------//
        //--- Lights Functions ---//
        //------------------------//

        LightId CreatePointLight(const glm::vec3& position, const glm::vec3& color, float intensity);

        void UpdatePointLight(LightId light_id, const glm::vec3& position, const glm::vec3& color, float intensity);

        LightId CreateDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity);

        void UpdateDirectionalLight(LightId light_id, const glm::vec3& direction, const glm::vec3& color,
                                    float intensity);

        LightId CreateSpotLight(const glm::vec3& position, float cutoff_angle, const glm::vec3& direction,
                                float intensity, const glm::vec3& color);

        void UpdateSpotLight(LightId light_id, const glm::vec3& position, float cutoff_angle,
                             const glm::vec3& direction,
                             float intensity, const glm::vec3& color);

        LightId CreateAmbientLight(const glm::vec3& color, float intensity);

        void UpdateAmbientLight(LightId light_id, const glm::vec3& color, float intensity);

        void RemoveLight(LightId light_id);

        //------------------------//
        //-- Viewport Functions --//
        //------------------------//

        ViewportId CreateViewport(glm::uvec2 viewport_size);

        void ResizeViewport(ViewportId viewport_id, glm::uvec2 new_size);

        void RemoveViewport(ViewportId viewport_id);

        //-------------------------//
        //-- Rendering Functions --//
        //-------------------------//

        void UpdateDirtyInstances();

        void Render3D(ViewportId viewport, render::Texture2DHandle render_target);

    private:

        struct Light
        {
            glm::vec3    light_position {0.0};
            glm::f32     light_intensity {0.0};
            glm::vec3    light_direction {0.0};
            glm::uint    light_type {0};
            glm::vec3    light_color {0.0};
            glm::f32     light_cutoff {0.0};
        };

        struct RenderData
        {
            RenderData() : m_owner_node(nullptr)
            {}

            RenderData (NodeComponent* owner_node) : m_owner_node(owner_node)
            {}

            NodeComponent* m_owner_node;

            render::VertexBufferHandle m_vertex_buffer_handle {};
            render::IndexBufferHandle m_index_buffer_handle {};
            bool m_vertices_dirty = true;
            bool m_indices_dirty  = true;

            uint32_t num_vertices = 0, num_indices = 0;

            render::DescriptorLayout m_model_descriptor_layout;
            std::array<bool, render::FRAME_LAG> m_uniform_dirty { true };
            std::array<render::DeviceBuffer, render::FRAME_LAG>       m_uniform_buffers{}; // model transform
            std::array<render::DescriptorSetHandle, render::FRAME_LAG>  m_descriptor_sets{};
        };

        void SetupCameraUniforms();
        void SetupMaterialUniforms();
        void InitRenderDataUniforms(RenderData& render_data);

        LightId CreateNewLight(Light&& new_light);

        void CreateVertexBuffer(std::vector<Vertex3>& vertex_buffer, RenderData& render_data);
        void CreateIndexBuffer(std::vector<uint32_t>& index_buffer, RenderData& render_data);
        void DestroyBuffers(RenderData& render_data);

        Scene* m_scene;
        render::VulkanRenderDevice* m_render_device = nullptr;

        struct Viewport
        {
            uint32_t width, heigth;
            std::array<render::Texture2DHandle, render::FRAME_LAG> color_attachment {};
            std::array<render::Texture2DHandle, render::FRAME_LAG> depth_attachment {};
        };

        ContiguousPool<Viewport> m_viewports;

        uint32_t m_current_buffer = 0;
        size_t m_current_frame = 0;

        ContiguousPool<Light> m_scene_lights;

        struct SceneUniformInfo
        {
            std::array<render::DeviceBuffer, render::FRAME_LAG>        m_camera_uniforms;
            std::array<render::DeviceBuffer, render::FRAME_LAG>        m_lights_buffers;
            std::array<render::DescriptorSetHandle, render::FRAME_LAG> m_descriptor_sets;
            std::array<bool, render::FRAME_LAG> m_light_storage_dirty { true };
            std::array<bool, render::FRAME_LAG> m_light_storage_size_changed { true };
        } m_camera_uniform_info;

        ContiguousPool<RenderData> m_render_data;

        render::DescriptorLayout m_material_descriptor_layout;
        std::array<render::DescriptorSetHandle, render::FRAME_LAG>  m_material_descriptor_sets{};

        std::unique_ptr<Image> m_image;
        render::Texture2DHandle m_texture_2d_handle;
        render::ResourceHandle m_graphics_pipeline;

        render::Shader m_shader;
    };

}

#endif
