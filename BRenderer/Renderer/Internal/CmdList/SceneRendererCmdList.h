#ifndef BRR_SCENERENDERERCMDLIST_H
#define BRR_SCENERENDERERCMDLIST_H

#include "CmdList.h"

#include <Renderer/SceneObjectsIDs.h>

#include <Core/thirdpartiesInc.h>

namespace brr::render::internal
{
    enum class SceneRendererCmdType
    {
        CreateSceneRenderer,
        DestroySceneRenderer,
        CreateEntity,
        DestroyEntity,
        UpdateEntityTransform,
        CreateSurface,
        DestroySurface,
        UpdateSurfaceVertexBuffer,
        UpdateSurfaceIndexBuffer,
        CreateCamera,
        DestroyCamera,
        UpdateCameraProjection,
        CreatePointLight,
        UpdatePointLight,
        CreateDirectionalLight,
        UpdateDirectionalLight,
        CreateSpotLight,
        UpdateSpotLight,
        CreateAmbientLight,
        UpdateAmbientLight,
        DestroyLight
    };

    struct SceneRendererCommand
    {
        static SceneRendererCommand BuildCreateSceneRendererCommand()
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type = SceneRendererCmdType::CreateSceneRenderer;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildDestroySceneRendererCommand()
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type = SceneRendererCmdType::DestroySceneRenderer;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateEntityCommand(EntityId entity_id,
                                                             const glm::mat4& entity_transform = {})
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                    = SceneRendererCmdType::CreateEntity;
            scene_rend_command.entity_command.entity_id        = entity_id;
            scene_rend_command.entity_command.entity_transform = entity_transform;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildDestroyEntityCommand(EntityId entity_id)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type             = SceneRendererCmdType::DestroyEntity;
            scene_rend_command.entity_command.entity_id = entity_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateEntityTransformCommand(EntityId entity_id,
                                                                      const glm::mat4& entity_transform)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                    = SceneRendererCmdType::UpdateEntityTransform;
            scene_rend_command.entity_command.entity_id        = entity_id;
            scene_rend_command.entity_command.entity_transform = entity_transform;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateSurfaceCommand(EntityId entity_id,
                                                              SurfaceId surface_id,
                                                              void* vertex_buffer,
                                                              size_t vertex_buffer_size,
                                                              void* index_buffer,
                                                              size_t index_buffer_size)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                       = SceneRendererCmdType::CreateSurface;
            scene_rend_command.surface_command.owner_entity_id    = entity_id;
            scene_rend_command.surface_command.surface_id         = surface_id;
            scene_rend_command.surface_command.vertex_buffer_data = malloc(vertex_buffer_size);
            scene_rend_command.surface_command.vertex_buffer_size = vertex_buffer_size;
            memcpy(scene_rend_command.surface_command.vertex_buffer_data, vertex_buffer, vertex_buffer_size);
            scene_rend_command.surface_command.index_buffer_data  = malloc(index_buffer_size);
            scene_rend_command.surface_command.index_buffer_size  = index_buffer_size;
            memcpy(scene_rend_command.surface_command.index_buffer_data, index_buffer, index_buffer_size);
            return scene_rend_command;
        }

        static SceneRendererCommand BuildDestroySurfaceCommand(SurfaceId surface_id)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type               = SceneRendererCmdType::DestroySurface;
            scene_rend_command.surface_command.surface_id = surface_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateSurfaceVertexBufferCommand(SurfaceId surface_id,
                                                                          void* vertex_buffer,
                                                                          size_t vertex_buffer_size)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::UpdateSurfaceVertexBuffer;
            scene_rend_command.surface_command.surface_id    = surface_id;
            scene_rend_command.surface_command.vertex_buffer_data = malloc(vertex_buffer_size);
            scene_rend_command.surface_command.vertex_buffer_size = vertex_buffer_size;
            memcpy(scene_rend_command.surface_command.vertex_buffer_data, vertex_buffer, vertex_buffer_size);
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateSurfaceIndexBufferCommand(SurfaceId surface_id,
                                                                         void* index_buffer,
                                                                         size_t index_buffer_size)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                 = SceneRendererCmdType::UpdateSurfaceIndexBuffer;
            scene_rend_command.surface_command.surface_id   = surface_id;
            scene_rend_command.surface_command.index_buffer_data  = malloc(index_buffer_size);
            scene_rend_command.surface_command.index_buffer_size  = index_buffer_size;
            memcpy(scene_rend_command.surface_command.index_buffer_data, index_buffer, index_buffer_size);
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateCameraCommand(CameraId camera_id,
                                                             EntityId owner_entity,
                                                             float camera_fovy,
                                                             float camera_near,
                                                             float camera_far)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                   = SceneRendererCmdType::CreateCamera;
            scene_rend_command.camera_command.camera_id       = camera_id;
            scene_rend_command.camera_command.owner_entity_id = owner_entity;
            scene_rend_command.camera_command.camera_fov_y    = camera_fovy;
            scene_rend_command.camera_command.camera_near     = camera_near;
            scene_rend_command.camera_command.camera_far      = camera_far;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildDestroyCameraCommand(CameraId camera_id)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type             = SceneRendererCmdType::DestroyCamera;
            scene_rend_command.camera_command = {};
            scene_rend_command.camera_command.camera_id = camera_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateCameraProjectionCommand(CameraId camera_id,
                                                                       float camera_fovy,
                                                                       float camera_near,
                                                                       float camera_far)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                = SceneRendererCmdType::UpdateCameraProjection;
            scene_rend_command.camera_command.camera_id    = camera_id;
            scene_rend_command.camera_command.camera_fov_y = camera_fovy;
            scene_rend_command.camera_command.camera_near  = camera_near;
            scene_rend_command.camera_command.camera_far   = camera_far;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreatePointLightCommand(LightId light_id,
                                                                 const glm::vec3& position,
                                                                 const glm::vec3& color,
                                                                 float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::CreatePointLight;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.light_position  = position;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdatePointLightCommand(LightId light_id,
                                                                 const glm::vec3& position,
                                                                 const glm::vec3& color,
                                                                 float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::UpdatePointLight;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.light_position  = position;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateDirectionalLightCommand(LightId light_id,
                                                                       const glm::vec3& direction,
                                                                       const glm::vec3& color,
                                                                       float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::CreateDirectionalLight;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.light_direction = direction;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateDirectionalLightCommand(LightId light_id,
                                                                       const glm::vec3& direction,
                                                                       const glm::vec3& color,
                                                                       float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::UpdateDirectionalLight;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.light_direction = direction;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateSpotLightCommand(LightId light_id,
                                                                const glm::vec3& position,
                                                                float cutoff_angle,
                                                                const glm::vec3& direction,
                                                                float intensity,
                                                                const glm::vec3& color)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::CreateSpotLight;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.light_position  = position;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_direction = direction;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_cutoff    = cutoff_angle;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateSpotLightCommand(LightId light_id,
                                                                const glm::vec3& position,
                                                                float cutoff_angle,
                                                                const glm::vec3& direction,
                                                                float intensity,
                                                                const glm::vec3& color)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::UpdateSpotLight;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.light_position  = position;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_direction = direction;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_cutoff    = cutoff_angle;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateAmbientLightCommand(LightId light_id,
                                                                   const glm::vec3& color,
                                                                   float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::CreateAmbientLight;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_color     = color;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateAmbientLightCommand(LightId light_id,
                                                                   const glm::vec3& color,
                                                                   float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::UpdateAmbientLight;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_color     = color;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildDestroyLightCommand(LightId light_id)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type           = SceneRendererCmdType::DestroyLight;
            scene_rend_command.light_command.light_id = light_id;
            return scene_rend_command;
        }

    public:
        SceneRendererCmdType command_type;

        union
        {
            struct
            {
                EntityId entity_id{EntityId::NULL_ID};
                glm::mat4 entity_transform{};
            } entity_command;

            struct
            {
                EntityId owner_entity_id{EntityId::NULL_ID};
                SurfaceId surface_id{SurfaceId::NULL_ID};
                void* vertex_buffer_data{nullptr};
                size_t vertex_buffer_size{0};
                void* index_buffer_data{nullptr};
                size_t index_buffer_size{0};
            } surface_command;

            struct
            {
                CameraId camera_id{CameraId::NULL_ID};
                EntityId owner_entity_id{EntityId::NULL_ID};
                float camera_fov_y{0};
                float camera_near{0};
                float camera_far{0};
            } camera_command;

            struct
            {
                glm::vec3 light_position{0.0};
                glm::f32 light_intensity{0.0};
                glm::vec3 light_direction{0.0};
                glm::f32 light_cutoff{0.0};
                glm::vec3 light_color{0.0};
                LightId light_id;
            } light_command;
        };

        SceneRendererCommand(const SceneRendererCommand& other)
        {
            *this = other;
        }

        SceneRendererCommand& operator=(const SceneRendererCommand& other)
        {
            this->command_type = other.command_type;
            switch (other.command_type)
            {
            case SceneRendererCmdType::CreateSceneRenderer:
            case SceneRendererCmdType::DestroySceneRenderer:
                break;
            case SceneRendererCmdType::CreateEntity:
            case SceneRendererCmdType::DestroyEntity:
            case SceneRendererCmdType::UpdateEntityTransform:
                this->entity_command = other.entity_command;
                break;
            case SceneRendererCmdType::CreateSurface:
            case SceneRendererCmdType::DestroySurface:
            case SceneRendererCmdType::UpdateSurfaceVertexBuffer:
            case SceneRendererCmdType::UpdateSurfaceIndexBuffer:
                this->surface_command = other.surface_command;
                this->surface_command.vertex_buffer_data = nullptr;
                if (other.surface_command.vertex_buffer_size > 0)
                {
                    this->surface_command.vertex_buffer_data = malloc(other.surface_command.vertex_buffer_size);
                    memcpy(this->surface_command.vertex_buffer_data, other.surface_command.vertex_buffer_data, other.surface_command.vertex_buffer_size);
                }
                this->surface_command.index_buffer_data = nullptr;
                if (other.surface_command.index_buffer_size > 0)
                {
                    this->surface_command.index_buffer_data = malloc(other.surface_command.index_buffer_size);
                    memcpy(this->surface_command.index_buffer_data, other.surface_command.index_buffer_data, other.surface_command.index_buffer_size);
                }
                this->surface_command.vertex_buffer_size = other.surface_command.vertex_buffer_size;
                this->surface_command.index_buffer_size = other.surface_command.index_buffer_size;
                break;
            case SceneRendererCmdType::CreateCamera:
            case SceneRendererCmdType::DestroyCamera:
            case SceneRendererCmdType::UpdateCameraProjection:
                this->camera_command = other.camera_command;
                break;
            case SceneRendererCmdType::CreatePointLight:
            case SceneRendererCmdType::UpdatePointLight:
            case SceneRendererCmdType::CreateDirectionalLight:
            case SceneRendererCmdType::UpdateDirectionalLight:
            case SceneRendererCmdType::CreateSpotLight:
            case SceneRendererCmdType::UpdateSpotLight:
            case SceneRendererCmdType::CreateAmbientLight:
            case SceneRendererCmdType::UpdateAmbientLight:
            case SceneRendererCmdType::DestroyLight:
                this->light_command = other.light_command;
                break;
            }
            return *this;
        }

        ~SceneRendererCommand()
        {
        }

    private:
        SceneRendererCommand()
            : command_type(),
              surface_command()
        {
        }
    };

    using SceneRendererCmdList = CmdList<SceneRendererCommand>;
}

#endif
