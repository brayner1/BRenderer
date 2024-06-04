#include "SceneView.h"

#include "Window.h"
#include "Core/LogSystem.h"
#include "Renderer/RenderThread.h"

namespace brr::vis
{
    SceneView::SceneView(Window* window)
    : m_window(window),
      m_scene(nullptr),
      m_camera_entity(Entity())
    {
        m_camera_destroyed_action = std::make_shared<EventAction<>>(&SceneView::OnCameraDestroyed, this);
        if (!m_window)
        {
            BRR_LogWarn("Initializing SceneView with NULL Window pointer. Aborting rendering setup.");
            return;
        }
    }

    SceneView::SceneView(Window* window, const PerspectiveCameraComponent* camera)
    : m_window(window),
      m_scene(camera->GetScene()),
      m_camera_entity(camera->GetEntity())
    {
        if (!m_window)
        {
            BRR_LogWarn("Initializing SceneView with NULL Window pointer. Aborting rendering setup.");
            return;
        }

        SetCamera(camera);
    }

    SceneView::~SceneView()
    {
        UnsetCamera();
    }

    PerspectiveCameraComponent* SceneView::GetCameraComponent() const
    {
        if (m_camera_entity)
        {
            return &(m_camera_entity.GetComponent<PerspectiveCameraComponent>());
        }
        return nullptr;
    }

    void SceneView::SetCamera(const PerspectiveCameraComponent* camera)
    {
        if (m_camera_entity)
        {
            PerspectiveCameraComponent& old_camera = m_camera_entity.GetComponent<PerspectiveCameraComponent>();
            old_camera.m_destroyed_event.Unsubscribe(m_camera_destroyed_action);
        }
        m_scene = camera ? camera->GetScene() : nullptr;
        m_camera_entity = camera ? camera->GetEntity() : Entity();

        uint64_t scene_id = -1;
        render::CameraID camera_id = render::CameraID::NULL_ID;
        if (m_scene)
        {
            if (SceneRenderProxy* scene_render_proxy = m_scene->GetSceneRendererProxy())
            {
                scene_id = scene_render_proxy->GetSceneID();
                camera_id = camera->GetCameraRenderID();

            }
        }

        if (camera)
            camera->m_destroyed_event.Subscribe(m_camera_destroyed_action);

        render::RenderThread::WindowRenderCmd_SetSceneView(m_window->GetSDLWindowHandle(), scene_id, camera_id);
    }

    void SceneView::UnsetCamera()
    {
        if (m_camera_entity)
        {
            PerspectiveCameraComponent& old_camera = m_camera_entity.GetComponent<PerspectiveCameraComponent>();
            old_camera.m_destroyed_event.Unsubscribe(m_camera_destroyed_action);
            render::RenderThread::WindowRenderCmd_SetSceneView(m_window->GetSDLWindowHandle(), -1, render::CameraID::NULL_ID);
            m_scene = nullptr;
            m_camera_entity = Entity();
        }
    }

    void SceneView::OnCameraDestroyed()
    {
        UnsetCamera();
    }
}
