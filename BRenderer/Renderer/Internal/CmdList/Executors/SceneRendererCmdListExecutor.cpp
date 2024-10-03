#include "SceneRendererCmdListExecutor.h"

namespace brr::render::internal
{
    void SceneRendererCmdListExecutor::ExecuteCmdList()
    {
        for (auto& scene_cmds_pair : m_scene_cmd_list_map)
        {
            uint64_t scene_id             = scene_cmds_pair.first;
            SceneRenderer* scene_renderer = nullptr;
            SystemOwner<SceneRenderer>& scene_renderer_storage = SystemsStorage::GetSceneRendererStorage();
            auto scene_renderer_iter = scene_renderer_storage.Find(scene_id);
            if (scene_renderer_iter != scene_renderer_storage.end())
            {
                scene_renderer = scene_renderer_iter->second.get();
            }

            for (auto& scene_cmd : scene_cmds_pair.second)
            {
                switch (scene_cmd.command_type)
                {
                case SceneRendererCmdType::CreateSceneRenderer:
                    {
                        if (scene_renderer)
                        {
                            BRR_LogError("SceneRenderer command error. Command: CreateSceneRenderer. "
                                         "Error: Can't create SceneRenderer (ID: {}) because it is already created",
                                         scene_id);
                            break;
                        }
                        scene_renderer = scene_renderer_storage.CreateNew(scene_id, std::make_unique<SceneRenderer>());
                        if (!scene_renderer)
                        {
                            BRR_LogError("SceneRenderer command failed. Command: CreateSceneRenderer. "
                                         "Error: Failed to allocate new SceneRenderer (ID: {})", scene_id);
                            break;
                        }
                        BRR_LogDebug("RenderThread: SceneRenderer (ID: {}) created.", scene_id);
                        break;
                    }
                case SceneRendererCmdType::DestroySceneRenderer:
                    {
                        if (!scene_renderer)
                        {
                            BRR_LogError("SceneRenderer command error. Command: DestroySceneRenderer. "
                                         "Error: Can't destroy SceneRenderer (ID: {}) because it does not exist.",
                                         scene_id);
                            break;
                        }

                        for (auto& window_renderer : SystemsStorage::GetWindowRendererStorage())
                        {
                            if (window_renderer.second->GetSceneRenderer() == scene_renderer)
                            {
                                window_renderer.second->SetSceneRenderer(nullptr, CameraID::NULL_ID);
                            }
                        }

                        scene_renderer_storage.Erase(scene_id);
                        scene_renderer = nullptr;
                        BRR_LogDebug("RenderThread: SceneRenderer (ID: {}) destroyed.", scene_id);
                        break;
                    }
                default:
                    ExecuteSceneRendererUpdateCommand(scene_cmd, scene_renderer);
                }
            }
        }
    }

    void SceneRendererCmdListExecutor::ExecuteSceneRendererUpdateCommand(const SceneRendererCommand& scene_command,
                                                                         SceneRenderer* scene_renderer)
    {
        if (!scene_renderer)
        {
            BRR_LogError("Can't execute SceneRenderer command with uninitialized SceneRenderer.");
            return;
        }
        switch (scene_command.command_type)
        {
        case SceneRendererCmdType::CreateSceneRenderer:
        case SceneRendererCmdType::DestroySceneRenderer:
            // Already handled
            break;
        case SceneRendererCmdType::CreateEntity:
            scene_renderer->CreateEntity(scene_command.entity_command.entity_id,
                                         scene_command.entity_command.entity_transform);
            break;
        case SceneRendererCmdType::DestroyEntity:
            scene_renderer->DestroyEntity(scene_command.entity_command.entity_id);
            break;
        case SceneRendererCmdType::UpdateEntityTransform:
            scene_renderer->UpdateEntityTransform(scene_command.entity_command.entity_id,
                                                  scene_command.entity_command.entity_transform);
            break;
        case SceneRendererCmdType::AppendSurface:
            scene_renderer->AppendSurfaceToEntity(scene_command.surface_command.surface_id,
                                                  scene_command.entity_command.entity_id);
            break;
        case SceneRendererCmdType::CreateCamera:
            scene_renderer->CreateCamera(scene_command.camera_command.camera_id,
                                         scene_command.camera_command.owner_entity_id,
                                         scene_command.camera_command.camera_fov_y,
                                         scene_command.camera_command.camera_near,
                                         scene_command.camera_command.camera_far);
            break;
        case SceneRendererCmdType::DestroyCamera:
            scene_renderer->DestroyCamera(scene_command.camera_command.camera_id);
            break;
        case SceneRendererCmdType::UpdateCameraProjection:
            scene_renderer->UpdateCameraProjection(scene_command.camera_command.camera_id,
                                                   scene_command.camera_command.camera_fov_y,
                                                   scene_command.camera_command.camera_near,
                                                   scene_command.camera_command.camera_far);
            break;
        case SceneRendererCmdType::CreatePointLight:
            scene_renderer->CreatePointLight(scene_command.light_command.light_id, scene_command.light_command.light_position, scene_command.light_command.light_color, scene_command.light_command.light_intensity);
            break;
        case SceneRendererCmdType::UpdatePointLight:
            scene_renderer->UpdatePointLight(scene_command.light_command.light_id, scene_command.light_command.light_position, scene_command.light_command.light_color, scene_command.light_command.light_intensity);
            break;
        case SceneRendererCmdType::CreateDirectionalLight:
            scene_renderer->CreateDirectionalLight(scene_command.light_command.light_id, scene_command.light_command.light_direction, scene_command.light_command.light_color, scene_command.light_command.light_intensity);
            break;
        case SceneRendererCmdType::UpdateDirectionalLight:
            scene_renderer->UpdateDirectionalLight(scene_command.light_command.light_id, scene_command.light_command.light_direction, scene_command.light_command.light_color, scene_command.light_command.light_intensity);
            break;
        case SceneRendererCmdType::CreateSpotLight:
            scene_renderer->CreateSpotLight(scene_command.light_command.light_id, scene_command.light_command.light_position, scene_command.light_command.light_cutoff, scene_command.light_command.light_direction, scene_command.light_command.light_intensity, scene_command.light_command.light_color);
            break;
        case SceneRendererCmdType::UpdateSpotLight:
            scene_renderer->UpdateSpotLight(scene_command.light_command.light_id, scene_command.light_command.light_position, scene_command.light_command.light_cutoff, scene_command.light_command.light_direction, scene_command.light_command.light_intensity, scene_command.light_command.light_color);
            break;
        case SceneRendererCmdType::CreateAmbientLight:
            scene_renderer->CreateAmbientLight(scene_command.light_command.light_id, scene_command.light_command.light_color, scene_command.light_command.light_intensity);
            break;
        case SceneRendererCmdType::UpdateAmbientLight:
            scene_renderer->UpdateAmbientLight(scene_command.light_command.light_id, scene_command.light_command.light_color, scene_command.light_command.light_intensity);
            break;
        case SceneRendererCmdType::DestroyLight:
            scene_renderer->DestroyLight(scene_command.light_command.light_id);
            break;
        }
    }
}
