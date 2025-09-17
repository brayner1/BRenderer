#ifndef BRR_SCENERENDERERCMDLIST_H
#define BRR_SCENERENDERERCMDLIST_H

#include "CmdList.h"

#include <Renderer/SceneObjectsIDs.h>
#include <Renderer/RenderingResourceIDs.h>

#include <Core/thirdpartiesInc.h>

namespace brr::render::internal
{
    enum class SceneRendererCmdType
    {
        // Scene Renderer
        CreateSceneRenderer,
        DestroySceneRenderer,
        // Entity
        CreateEntity,
        DestroyEntity,
        UpdateEntityTransform,
        AppendSurface,
        // Camera
        CreateCamera,
        DestroyCamera,
        UpdateCameraProjection,
        // Lights
        CreatePointLight,
        CreateDirectionalLight,
        CreateSpotLight,
        CreateAmbientLight,
        UpdateLight,
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

        static SceneRendererCommand BuildCreateEntityCommand(EntityID entity_id,
                                                             const glm::mat4& entity_transform = {})
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                    = SceneRendererCmdType::CreateEntity;
            scene_rend_command.entity_command.entity_id        = entity_id;
            scene_rend_command.entity_command.entity_transform = entity_transform;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildDestroyEntityCommand(EntityID entity_id)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type             = SceneRendererCmdType::DestroyEntity;
            scene_rend_command.entity_command.entity_id = entity_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateEntityTransformCommand(EntityID entity_id,
                                                                      const glm::mat4& entity_transform)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                    = SceneRendererCmdType::UpdateEntityTransform;
            scene_rend_command.entity_command.entity_id        = entity_id;
            scene_rend_command.entity_command.entity_transform = entity_transform;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildAppendSurfaceCommand(EntityID entity_id,
                                                              SurfaceID surface_id)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                       = SceneRendererCmdType::AppendSurface;
            scene_rend_command.surface_command.owner_entity_id    = entity_id;
            scene_rend_command.surface_command.surface_id         = surface_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateCameraCommand(CameraID camera_id,
                                                             EntityID owner_entity,
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

        static SceneRendererCommand BuildDestroyCameraCommand(CameraID camera_id)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type             = SceneRendererCmdType::DestroyCamera;
            scene_rend_command.camera_command = {};
            scene_rend_command.camera_command.camera_id = camera_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateCameraProjectionCommand(CameraID camera_id,
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

        static SceneRendererCommand BuildCreatePointLightCommand(LightID light_id,
                                                                 EntityID owner_entity_id,
                                                                 const glm::vec3& color,
                                                                 float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::CreatePointLight;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.owner_entity_id = owner_entity_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateDirectionalLightCommand(LightID light_id,
                                                                       EntityID owner_entity_id,
                                                                       const glm::vec3& color,
                                                                       float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::CreateDirectionalLight;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.owner_entity_id = owner_entity_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateSpotLightCommand(LightID light_id,
                                                                EntityID owner_entity_id,
                                                                const glm::vec3& color,
                                                                float intensity,
                                                                float cutoff_angle)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::CreateSpotLight;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_cutoff    = cutoff_angle;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.owner_entity_id = owner_entity_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildCreateAmbientLightCommand(LightID light_id,
                                                                   EntityID owner_entity_id,
                                                                   const glm::vec3& color,
                                                                   float intensity)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::CreateAmbientLight;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_id        = light_id;
            scene_rend_command.light_command.owner_entity_id = owner_entity_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildUpdateLightCommand(LightID light_id,
                                                            const glm::vec3& color,
                                                            float intensity,
                                                            float cutoff_angle)
        {
            SceneRendererCommand scene_rend_command;
            scene_rend_command.command_type                  = SceneRendererCmdType::UpdateLight;
            scene_rend_command.light_command.light_color     = color;
            scene_rend_command.light_command.light_intensity = intensity;
            scene_rend_command.light_command.light_cutoff    = cutoff_angle;
            scene_rend_command.light_command.light_id        = light_id;
            return scene_rend_command;
        }

        static SceneRendererCommand BuildDestroyLightCommand(LightID light_id)
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
                EntityID entity_id{EntityID::NULL_ID};
                glm::mat4 entity_transform{};
            } entity_command;

            struct
            {
                EntityID owner_entity_id{EntityID::NULL_ID};
                SurfaceID surface_id{};
            } surface_command;

            struct
            {
                CameraID camera_id{CameraID::NULL_ID};
                EntityID owner_entity_id{EntityID::NULL_ID};
                float camera_fov_y{0};
                float camera_near{0};
                float camera_far{0};
            } camera_command;

            struct
            {
                glm::vec3 light_color{0.0};
                glm::f32 light_intensity{0.0};
                glm::f32 light_cutoff{0.0};
                LightID light_id{LightID::NULL_ID};
                EntityID owner_entity_id{EntityID::NULL_ID};
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
            case SceneRendererCmdType::AppendSurface:
                this->surface_command = other.surface_command;
                break;
            case SceneRendererCmdType::CreateCamera:
            case SceneRendererCmdType::DestroyCamera:
            case SceneRendererCmdType::UpdateCameraProjection:
                this->camera_command = other.camera_command;
                break;
            case SceneRendererCmdType::CreatePointLight:
            case SceneRendererCmdType::CreateDirectionalLight:
            case SceneRendererCmdType::CreateSpotLight:
            case SceneRendererCmdType::CreateAmbientLight:
            case SceneRendererCmdType::UpdateLight:
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
