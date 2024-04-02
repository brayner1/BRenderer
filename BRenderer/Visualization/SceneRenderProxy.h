#ifndef BRR_SCENERENDERPROXY_H
#define BRR_SCENERENDERPROXY_H

namespace brr::vis
{
    class SceneRenderProxy
    {
    public:

        SceneRenderProxy(vis::Window* owner_window);

        ~SceneRenderProxy();

        // Camera
        void CreateCamera(UUID camera_uuid);

        void UpdateCameraViewMatrix(UUID camera_uuid, const glm::mat4& camera_view);

        void UpdateCameraProjectionMatrix(UUID camera_uuid, const glm::mat4& camera_projection);

        // Render Entity
        void CreateRenderEntity(Entity owner_entity);

        void UpdateRenderEntityTransform(Entity owner_entity, const glm::mat4& entity_transform);

        // Surface
        void CreateSurface(Entity owner_entity, UUID surface_uuid, void* vertex_buffer_data, size_t vertex_buffer_size, void* index_buffer_data, size_t index_buffer_size);

        void RemoveSurface(UUID surface_uuid);

        // Lights
        using LightId = uint64_t;

        void CreatePointLight(LightId light_id, const glm::vec3& position, const glm::vec3& color, float intensity);

        void UpdatePointLight(LightId light_id, const glm::vec3& position, const glm::vec3& color, float intensity);

        void CreateDirectionalLight(LightId light_id, const glm::vec3& direction, const glm::vec3& color, float intensity);

        void UpdateDirectionalLight(LightId light_id, const glm::vec3& direction, const glm::vec3& color,
                                    float intensity);

        void CreateSpotLight(LightId light_id, const glm::vec3& position, float cutoff_angle, const glm::vec3& direction,
                                float intensity, const glm::vec3& color);

        void UpdateSpotLight(LightId light_id, const glm::vec3& position, float cutoff_angle,
                             const glm::vec3& direction,
                             float intensity, const glm::vec3& color);

        void CreateAmbientLight(LightId light_id, const glm::vec3& color, float intensity);

        void UpdateAmbientLight(LightId light_id, const glm::vec3& color, float intensity);

        void RemoveLight(LightId light_id);          
    };
}

#endif