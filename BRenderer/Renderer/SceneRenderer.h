#ifndef BRR_SCENERENDERER_H
#define BRR_SCENERENDERER_H
#include <map>
#include <Core/Storage/ContiguousPool.h>
#include <Renderer/Descriptors.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/RenderDefs.h>
#include <Renderer/SceneObjectsIDs.h>
#include <Scene/Scene.h>
#include <Visualization/Image.h>

// TODO: How do Camera, Viewport and Window connect with each other?

namespace brr::render
{
    class SceneRenderer
    {
    public:
        SceneRenderer();

        ~SceneRenderer();

        //------------------------//
        //--- Camera Functions ---//
        //------------------------//

        void CreateCamera(CameraId camera_id,
                          EntityId owner_entity,
                          float camera_fovy,
                          float camera_near,
                          float camera_far);
        void DestroyCamera(CameraId camera_id);
        void UpdateCameraProjection(CameraId camera_id,
                                    float camera_fovy,
                                    float camera_near,
                                    float camera_far);

        //------------------------//
        //--- Entity Functions ---//
        //------------------------//

        void CreateEntity(EntityId entity_id,
                          const glm::mat4& entity_transform = {});
        void DestroyEntity(EntityId entity_id);
        void UpdateEntityTransform(EntityId entity_id,
                                   const glm::mat4& entity_transform);

        //-------------------------//
        //--- Surface Functions ---//
        //-------------------------//

        void CreateSurface(SurfaceId surface_id,
                           EntityId owner_entity,
                           void* vertex_buffer_data,
                           size_t vertex_buffer_size,
                           void* index_buffer_data,
                           size_t index_buffer_size);

        void DestroySurface(SurfaceId surface_id);

        void UpdateSurfaceVertexBuffer(SurfaceId surface_id,
                                       void* vertex_buffer_data,
                                       size_t vertex_buffer_size);
        void UpdateSurfaceIndexBuffer(SurfaceId surface_id,
                                      void* index_buffer_data,
                                      size_t index_buffer_size);

        //------------------------//
        //--- Lights Functions ---//
        //------------------------//

        void CreatePointLight(LightId light_id,
                              const glm::vec3& position,
                              const glm::vec3& color,
                              float intensity);

        void UpdatePointLight(LightId light_id,
                              const glm::vec3& position,
                              const glm::vec3& color,
                              float intensity);

        void CreateDirectionalLight(LightId light_id, const glm::vec3& direction,
                                       const glm::vec3& color,
                                       float intensity);

        void UpdateDirectionalLight(LightId light_id,
                                    const glm::vec3& direction,
                                    const glm::vec3& color,
                                    float intensity);

        void CreateSpotLight(LightId light_id, const glm::vec3& position,
                                float cutoff_angle,
                                const glm::vec3& direction,
                                float intensity,
                                const glm::vec3& color);

        void UpdateSpotLight(LightId light_id,
                             const glm::vec3& position,
                             float cutoff_angle,
                             const glm::vec3& direction,
                             float intensity,
                             const glm::vec3& color);

        void CreateAmbientLight(LightId light_id, const glm::vec3& color,
                                   float intensity);

        void UpdateAmbientLight(LightId light_id,
                                const glm::vec3& color,
                                float intensity);

        void DestroyLight(LightId light_id);

        //------------------------//
        //-- Viewport Functions --//
        //------------------------//

        ViewportId CreateViewport(glm::uvec2 viewport_size, CameraId camera_id);

        void ResizeViewport(ViewportId viewport_id,
                            glm::uvec2 new_size);

        void DestroyViewport(ViewportId viewport_id);

        //-------------------------//
        //-- Rendering Functions --//
        //-------------------------//

        void UpdateDirtyInstances();

        void Render3D(ViewportId viewport,
                      Texture2DHandle render_target);

    private:
        struct Viewport;
        struct Light;
        struct EntityInfo;
        struct SurfaceRenderData;

        void SetupSceneUniforms();
        void SetupViewportUniforms(Viewport& viewport);
        void SetupEntityUniforms(EntityInfo& entity_info);
        void SetupMaterialUniforms();


        bool CreateNewLight(LightId light_id, Light&& new_light);

        void CreateVertexBuffer(void* vertex_buffer_data,
                                size_t vertex_buffer_size,
                                SurfaceRenderData& render_data);
        void CreateIndexBuffer(void* index_buffer_data,
                               size_t index_buffer_size,
                               SurfaceRenderData& render_data);
        void DestroySurfaceBuffers(SurfaceRenderData& render_data);

        //Scene* m_scene;
        VulkanRenderDevice* m_render_device = nullptr;

        struct Viewport
        {
            uint32_t width = 0, height = 0;
            CameraId camera_id;
            std::array<Texture2DHandle, FRAME_LAG> color_attachment{};
            std::array<Texture2DHandle, FRAME_LAG> depth_attachment{};

            // Camera matrices uniforms
            std::array<DeviceBuffer, FRAME_LAG> camera_uniform_buffers;
            std::array<DescriptorSetHandle, FRAME_LAG> camera_descriptor_sets;
            std::array<bool, FRAME_LAG> camera_uniform_dirty{true};
        };

        struct CameraInfo
        {
            float camera_fov_y;
            float camera_near;
            float camera_far;

            EntityId owner_entity = EntityId::NULL_ID;

            //std::array<DeviceBuffer, FRAME_LAG> uniform_buffers;
            //std::array<DescriptorSetHandle, FRAME_LAG> descriptor_sets;
            //std::array<bool, FRAME_LAG> uniform_dirty{true};
        };

        struct Light
        {
            glm::vec3 light_position{0.0};
            glm::f32 light_intensity{0.0};
            glm::vec3 light_direction{0.0};
            glm::uint light_type{0};
            glm::vec3 light_color{0.0};
            glm::f32 light_cutoff{0.0};
        };

        struct EntityInfo
        {
            glm::mat4 current_matrix;

            // model matrix uniforms
            DescriptorLayout descriptor_layout;
            std::array<DeviceBuffer, FRAME_LAG> uniform_buffers;
            std::array<DescriptorSetHandle, FRAME_LAG> descriptor_sets;
            std::array<bool, FRAME_LAG> uniform_dirty{true};
        };

        struct SurfaceRenderData
        {
            SurfaceRenderData()
                : m_owner_node(EntityId::NULL_ID)
            {
            }

            SurfaceRenderData(EntityId owner_node_id)
                : m_owner_node(owner_node_id)
            {
            }

            EntityId m_owner_node;
            SurfaceId m_my_surface_id;

            VertexBufferHandle m_vertex_buffer_handle{};
            IndexBufferHandle m_index_buffer_handle{};
            bool m_vertices_dirty = true;
            bool m_indices_dirty  = true;

            uint32_t num_vertices = 0, num_indices = 0;
        };

        struct SceneUniformInfo
        {
            std::array<DeviceBuffer, FRAME_LAG> m_lights_buffers;
            std::array<DescriptorSetHandle, FRAME_LAG> m_descriptor_sets;
            std::array<bool, FRAME_LAG> m_light_storage_dirty{true};
            std::array<bool, FRAME_LAG> m_light_storage_size_changed{true};
        } m_scene_uniform_info;

        ContiguousPool<ViewportId, Viewport> m_viewports;

        ContiguousPool<CameraId, CameraInfo> m_cameras;

        ContiguousPool<LightId, Light> m_scene_lights;

        std::map<EntityId, EntityInfo> m_entities_map;

        ContiguousPool<SurfaceId, SurfaceRenderData> m_render_data;

        DescriptorLayout m_material_descriptor_layout;
        std::array<DescriptorSetHandle, FRAME_LAG> m_material_descriptor_sets{};

        std::unique_ptr<vis::Image> m_image;
        Texture2DHandle m_texture_2d_handle;
        ResourceHandle m_graphics_pipeline;

        Shader m_shader;

        uint32_t m_current_buffer = 0;
        size_t m_current_frame    = 0;

        CameraId m_camera_id = CameraId::NULL_ID;
    };
}

#endif
