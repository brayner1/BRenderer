#ifndef BRR_SCENERENDERER_H
#define BRR_SCENERENDERER_H
#include <map>
#include <Core/Ref.h>
#include <Core/Storage/ContiguousPool.h>
#include <Renderer/GpuResources/Descriptors.h>
#include <Renderer/GpuResources/DeviceBuffer.h>
#include <Renderer/RenderDefs.h>
#include <Renderer/SceneObjectsIDs.h>
#include <Scene/Scene.h>
#include <Visualization/Resources/Image.h>

#include "Storages/MeshStorage.h"

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

        void CreateCamera(CameraID camera_id,
                          EntityID owner_entity,
                          float camera_fovy,
                          float camera_near,
                          float camera_far);
        void DestroyCamera(CameraID camera_id);
        void UpdateCameraProjection(CameraID camera_id,
                                    float camera_fovy,
                                    float camera_near,
                                    float camera_far);

        //------------------------//
        //--- Entity Functions ---//
        //------------------------//

        void CreateEntity(EntityID entity_id,
                          const glm::mat4& entity_transform = {});
        void DestroyEntity(EntityID entity_id);
        void UpdateEntityTransform(EntityID entity_id,
                                   const glm::mat4& entity_transform);

        //-------------------------//
        //--- Surface Functions ---//
        //-------------------------//

        void AppendSurfaceToEntity(SurfaceID surface_id, EntityID owner_entity);

        void NotifySurfaceChanged(SurfaceID surface_id);

        //------------------------//
        //--- Lights Functions ---//
        //------------------------//

        void CreatePointLight(LightID light_id,
                              const glm::vec3& position,
                              const glm::vec3& color,
                              float intensity);

        void UpdatePointLight(LightID light_id,
                              const glm::vec3& position,
                              const glm::vec3& color,
                              float intensity);

        void CreateDirectionalLight(LightID light_id, const glm::vec3& direction,
                                       const glm::vec3& color,
                                       float intensity);

        void UpdateDirectionalLight(LightID light_id,
                                    const glm::vec3& direction,
                                    const glm::vec3& color,
                                    float intensity);

        void CreateSpotLight(LightID light_id, const glm::vec3& position,
                                float cutoff_angle,
                                const glm::vec3& direction,
                                float intensity,
                                const glm::vec3& color);

        void UpdateSpotLight(LightID light_id,
                             const glm::vec3& position,
                             float cutoff_angle,
                             const glm::vec3& direction,
                             float intensity,
                             const glm::vec3& color);

        void CreateAmbientLight(LightID light_id, const glm::vec3& color,
                                   float intensity);

        void UpdateAmbientLight(LightID light_id,
                                const glm::vec3& color,
                                float intensity);

        void DestroyLight(LightID light_id);

        //------------------------//
        //-- Viewport Functions --//
        //------------------------//

        ViewportID CreateViewport(glm::uvec2 viewport_size, CameraID camera_id);

        void ResizeViewport(ViewportID viewport_id,
                            glm::uvec2 new_size);

        void DestroyViewport(ViewportID viewport_id);

        CameraID GetViewportCameraID(ViewportID viewport_id) const;

        void SetViewportCameraID(ViewportID viewport_id, CameraID camera_id);

        //-------------------------//
        //-- Rendering Functions --//
        //-------------------------//

        void UpdateDirtyInstances();

        void Render3D(ViewportID viewport,
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

        void MarkEntityDirty(EntityID entity_id, EntityInfo& entity_info, bool mark_surface, bool mark_uniform);

        bool CreateNewLight(LightID light_id, Light&& new_light);

        VulkanRenderDevice* m_render_device = nullptr;

        struct Viewport
        {
            uint32_t width = 0, height = 0;
            CameraID camera_id;
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

            EntityID owner_entity = EntityID::NULL_ID;

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

            std::vector<SurfaceID> surfaces;
            bool surfaces_dirty = false;
        };

        struct SurfaceRenderData
        {
            SurfaceRenderData()
                : m_owner_nodes()
            {
            }

            SurfaceRenderData(EntityID owner_node_id)
                : m_owner_nodes(1, owner_node_id)
            {
            }

            std::vector<EntityID> m_owner_nodes;
            SurfaceID m_my_surface_id;

            VertexBufferHandle m_vertex_buffer_handle{};
            IndexBufferHandle m_index_buffer_handle{};

            uint32_t m_num_vertices = 0, m_num_indices = 0;

            bool m_surface_dirty = false;
        };

        struct SceneUniformInfo
        {
            std::array<DeviceBuffer, FRAME_LAG> m_lights_buffers;
            std::array<DescriptorSetHandle, FRAME_LAG> m_descriptor_sets;
            std::array<bool, FRAME_LAG> m_light_storage_dirty{true};
            std::array<bool, FRAME_LAG> m_light_storage_size_changed{true};
        } m_scene_uniform_info;

        ContiguousPool<ViewportID, Viewport> m_viewports;

        ContiguousPool<CameraID, CameraInfo> m_cameras;

        ContiguousPool<LightID, Light> m_scene_lights;

        ContiguousPool<size_t, SurfaceRenderData> m_cached_surfaces;

        std::set<EntityID> m_dirty_entities {};
        std::map<EntityID, EntityInfo> m_entities_map {};

        DescriptorLayout m_material_descriptor_layout;
        std::array<DescriptorSetHandle, FRAME_LAG> m_material_descriptor_sets{};

        Ref<vis::Image> m_image;
        Texture2DHandle m_texture_2d_handle;
        ResourceHandle m_graphics_pipeline;

        Shader m_shader;

        uint32_t m_current_buffer = 0;
        size_t m_current_frame    = -1; // Start with invalid value.

        CameraID m_camera_id = CameraID::NULL_ID;
    };
}

#endif
