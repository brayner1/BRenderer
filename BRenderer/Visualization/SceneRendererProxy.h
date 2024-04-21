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

        uint64_t GetSceneId() const { return m_scene_renderer_id; }

        void FlushUpdateCommands();

        // Camera
        render::CameraId CreateCamera(const Transform3DComponent& owner_entity,
                                      float camera_fovy,
                                      float camera_near,
                                      float camera_far) const;

        void DestroyCamera(render::CameraId camera_id) const;

        void UpdateCameraProjectionMatrix(render::CameraId camera_id,
                                          float camera_fovy,
                                          float camera_near,
                                          float camera_far) const;

        // Render Entity
        render::EntityId CreateRenderEntity(const Transform3DComponent& entity_transform) const;

        void DestroyRenderEntity(const Transform3DComponent& entity_transform) const;

        void UpdateRenderEntityTransform(const Transform3DComponent& entity_transform);

        // Surface

        render::SurfaceId CreateSurface(const Transform3DComponent& owner_entity,
                                        void* vertex_buffer_data,
                                        size_t vertex_buffer_size,
                                        void* index_buffer_data,
                                        size_t index_buffer_size) const;

        void DestroySurface(render::SurfaceId surface_id) const;

        // Lights

        render::LightId CreatePointLight(const glm::vec3& position,
                                         const glm::vec3& color,
                                         float intensity) const;

        void UpdatePointLight(render::LightId light_id,
                              const glm::vec3& position,
                              const glm::vec3& color,
                              float intensity) const;

        render::LightId CreateDirectionalLight(const glm::vec3& direction,
                                               const glm::vec3& color,
                                               float intensity) const;

        void UpdateDirectionalLight(render::LightId light_id,
                                    const glm::vec3& direction,
                                    const glm::vec3& color,
                                    float intensity) const;

        render::LightId CreateSpotLight(const glm::vec3& position,
                                        float cutoff_angle,
                                        const glm::vec3& direction,
                                        float intensity,
                                        const glm::vec3& color) const;

        void UpdateSpotLight(render::LightId light_id,
                             const glm::vec3& position,
                             float cutoff_angle,
                             const glm::vec3& direction,
                             float intensity,
                             const glm::vec3& color) const;

        render::LightId CreateAmbientLight(const glm::vec3& color,
                                           float intensity) const;

        void UpdateAmbientLight(render::LightId light_id,
                                const glm::vec3& color,
                                float intensity) const;

        void DestroyLight(render::LightId light_id) const;

    private:
        uint64_t m_scene_renderer_id;

        using EntityUpdatePair = std::pair<render::EntityId, glm::mat4>;
        std::vector<EntityUpdatePair> m_entity_updates;
        std::unordered_map<render::EntityId, uint32_t> m_entity_updates_idx_map;
    };
}

#endif
