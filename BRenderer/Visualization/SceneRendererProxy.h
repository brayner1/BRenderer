#ifndef BRR_SCENERENDERPROXY_H
#define BRR_SCENERENDERPROXY_H

#include <Core/thirdpartiesInc.h>
#include <cstdint>

#include <Renderer/SceneObjectsIDs.h>

namespace brr
{
    struct Transform3DComponent;
}

namespace brr::vis
{
    class SceneRenderProxy
    {
    public:

        SceneRenderProxy();

        ~SceneRenderProxy();

        uint64_t GetSceneID() const { return m_scene_renderer_id; }

        void FlushUpdateCommands();

        // Camera
        render::CameraID CreateCamera(const Transform3DComponent& owner_entity,
                                      float camera_fovy,
                                      float camera_near,
                                      float camera_far) const;

        void DestroyCamera(render::CameraID camera_id) const;

        void UpdateCameraProjectionMatrix(render::CameraID camera_id,
                                          float camera_fovy,
                                          float camera_near,
                                          float camera_far) const;

        // Render Entity
        render::EntityID CreateRenderEntity(const Transform3DComponent& entity_transform) const;

        void DestroyRenderEntity(const Transform3DComponent& entity_transform) const;

        void UpdateRenderEntityTransform(const Transform3DComponent& entity_transform);

        // Surface

        render::SurfaceID CreateSurface(const Transform3DComponent& owner_entity,
                                        void* vertex_buffer_data,
                                        size_t vertex_buffer_size,
                                        void* index_buffer_data,
                                        size_t index_buffer_size) const;

        void DestroySurface(render::SurfaceID surface_id) const;

        // Lights

        render::LightID CreatePointLight(const glm::vec3& position,
                                         const glm::vec3& color,
                                         float intensity) const;

        void UpdatePointLight(render::LightID light_id,
                              const glm::vec3& position,
                              const glm::vec3& color,
                              float intensity) const;

        render::LightID CreateDirectionalLight(const glm::vec3& direction,
                                               const glm::vec3& color,
                                               float intensity) const;

        void UpdateDirectionalLight(render::LightID light_id,
                                    const glm::vec3& direction,
                                    const glm::vec3& color,
                                    float intensity) const;

        render::LightID CreateSpotLight(const glm::vec3& position,
                                        float cutoff_angle,
                                        const glm::vec3& direction,
                                        float intensity,
                                        const glm::vec3& color) const;

        void UpdateSpotLight(render::LightID light_id,
                             const glm::vec3& position,
                             float cutoff_angle,
                             const glm::vec3& direction,
                             float intensity,
                             const glm::vec3& color) const;

        render::LightID CreateAmbientLight(const glm::vec3& color,
                                           float intensity) const;

        void UpdateAmbientLight(render::LightID light_id,
                                const glm::vec3& color,
                                float intensity) const;

        void DestroyLight(render::LightID light_id) const;

    private:
        uint64_t m_scene_renderer_id;

        using EntityUpdatePair = std::pair<render::EntityID, glm::mat4>;
        std::vector<EntityUpdatePair> m_entity_updates;
        std::unordered_map<render::EntityID, uint32_t> m_entity_updates_idx_map;
    };
}

#endif
