#include "SceneRendererProxy.h"

#include "Renderer/RenderThread.h"
#include "Scene/Components/Transform3DComponent.h"

using namespace brr::render;

namespace brr::vis
{
    SceneRenderProxy::SceneRenderProxy()
    {
        m_scene_renderer_id = RenderThread::RenderCmd_InitializeSceneRenderer();
    }

    SceneRenderProxy::~SceneRenderProxy()
    {
        RenderThread::RenderCmd_DestroySceneRenderer(m_scene_renderer_id);
    }

    // Camera

    void SceneRenderProxy::FlushUpdateCommands()
    {
        for (EntityUpdatePair& entity_update_pair : m_entity_updates)
        {
            RenderThread::SceneRenderCmd_UpdateEntityTransform(m_scene_renderer_id, entity_update_pair.first,
                                                               entity_update_pair.second);
        }
        m_entity_updates.Clear();

        for (CameraUpdateData& camera_update_data : m_camera_updates)
        {
            RenderThread::SceneRenderCmd_UpdateCameraProjection(m_scene_renderer_id, camera_update_data.camera_id,
                                                                camera_update_data.camera_fov_y, camera_update_data.camera_near,
                                                                camera_update_data.camera_far);
        }
        m_camera_updates.Clear();
    }

    CameraID SceneRenderProxy::CreateCamera(const Transform3DComponent& owner_entity,
                                            float camera_fovy,
                                            float camera_near,
                                            float camera_far) const
    {
        
        return RenderThread::SceneRenderCmd_CreateCamera(m_scene_renderer_id, owner_entity.GetRenderEntityID(), camera_fovy, camera_near, camera_far);
    }

    void SceneRenderProxy::DestroyCamera(CameraID camera_id) const
    {
        RenderThread::SceneRenderCmd_DestroyCamera(m_scene_renderer_id, camera_id);
    }

    void SceneRenderProxy::UpdateCameraProjectionMatrix(CameraID camera_id,
                                                        float camera_fovy,
                                                        float camera_near,
                                                        float camera_far)
    {
        CameraUpdateData update_data = {camera_id, camera_fovy, camera_near, camera_far};
        auto camera_update_data = m_camera_updates.Find(camera_id);
        if (camera_update_data == m_camera_updates.end())
        {
            m_camera_updates.AddObject(camera_id, update_data);
        }
        else
        {
            *camera_update_data = update_data;
        }
    }

    // Entities

    EntityID SceneRenderProxy::CreateRenderEntity(const Transform3DComponent& entity_transform) const
    {
        return RenderThread::SceneRenderCmd_CreateEntity(m_scene_renderer_id, entity_transform.GetGlobalMatrix());
    }

    void SceneRenderProxy::DestroyRenderEntity(const Transform3DComponent& entity_transform) const
    {
        RenderThread::SceneRenderCmd_DestroyEntity(m_scene_renderer_id, entity_transform.GetRenderEntityID());
    }

    void SceneRenderProxy::UpdateRenderEntityTransform(const Transform3DComponent& entity_transform)
    {
        EntityID entity_id = entity_transform.GetRenderEntityID();
        EntityUpdatePair update_pair = {entity_id, entity_transform.GetGlobalMatrix()};
        auto entity_update_iter = m_entity_updates.Find(entity_id);
        if (entity_update_iter == m_entity_updates.end())
        {
            m_entity_updates.AddObject(entity_id, update_pair);
        }
        else
        {
            *entity_update_iter = update_pair;
        }
    }

    // Surfaces

    SurfaceID SceneRenderProxy::CreateSurface(const Transform3DComponent& owner_entity,
                                              void* vertex_buffer_data,
                                              size_t vertex_buffer_size,
                                              void* index_buffer_data,
                                              size_t index_buffer_size) const
    {
        return render::RenderThread::SceneRenderCmd_CreateSurface(m_scene_renderer_id, owner_entity.GetRenderEntityID(),
                                                                  vertex_buffer_data, vertex_buffer_size, index_buffer_data,
                                                                  index_buffer_size);
    }

    void SceneRenderProxy::DestroySurface(SurfaceID surface_id) const
    {
        RenderThread::SceneRenderCmd_DestroySurface(m_scene_renderer_id, surface_id);
    }

    // Lights

    LightID SceneRenderProxy::CreatePointLight(const glm::vec3& position,
                                               const glm::vec3& color,
                                               float intensity) const
    {
        return RenderThread::SceneRenderCmd_CreatePointLight(m_scene_renderer_id, position, color, intensity);
    }

    void SceneRenderProxy::UpdatePointLight(LightID light_id,
                                            const glm::vec3& position,
                                            const glm::vec3& color,
                                            float intensity) const
    {
        RenderThread::SceneRenderCmd_UpdatePointLight(m_scene_renderer_id, light_id, position, color, intensity);
    }

    LightID SceneRenderProxy::CreateDirectionalLight(const glm::vec3& direction,
                                                     const glm::vec3& color,
                                                     float intensity) const
    {
        return RenderThread::SceneRenderCmd_CreateDirectionalLight(m_scene_renderer_id, direction, color, intensity);
    }

    void SceneRenderProxy::UpdateDirectionalLight(LightID light_id,
                                                  const glm::vec3& direction,
                                                  const glm::vec3& color,
                                                  float intensity) const
    {
        RenderThread::SceneRenderCmd_UpdateDirectionalLight(m_scene_renderer_id, light_id, direction, color, intensity);
    }

    LightID SceneRenderProxy::CreateSpotLight(const glm::vec3& position,
                                              float cutoff_angle,
                                              const glm::vec3& direction,
                                              float intensity,
                                              const glm::vec3& color) const
    {
        return RenderThread::SceneRenderCmd_CreateSpotLight(m_scene_renderer_id, position, cutoff_angle, direction, intensity,
                                                       color);
    }

    void SceneRenderProxy::UpdateSpotLight(LightID light_id,
                                           const glm::vec3& position,
                                           float cutoff_angle,
                                           const glm::vec3& direction,
                                           float intensity,
                                           const glm::vec3& color) const
    {
        RenderThread::SceneRenderCmd_UpdateSpotLight(m_scene_renderer_id, light_id, position, cutoff_angle, direction, intensity, color);
    }

    LightID SceneRenderProxy::CreateAmbientLight(const glm::vec3& color,
                                                 float intensity) const
    {
        return RenderThread::SceneRenderCmd_CreateAmbientLight(m_scene_renderer_id, color, intensity);
    }

    void SceneRenderProxy::UpdateAmbientLight(LightID light_id,
                                              const glm::vec3& color,
                                              float intensity) const
    {
        RenderThread::SceneRenderCmd_UpdateAmbientLight(m_scene_renderer_id, light_id, color, intensity);
    }

    void SceneRenderProxy::DestroyLight(LightID light_id) const
    {
        RenderThread::SceneRenderCmd_DestroyLight(m_scene_renderer_id, light_id);
    }
}
