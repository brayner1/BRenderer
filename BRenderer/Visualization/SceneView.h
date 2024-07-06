#ifndef BRR_SCENEVIEW_H
#define BRR_SCENEVIEW_H

#include <Core/Events/Event.h>

#include <Scene/Scene.h>
#include <Scene/Components/PerspectiveCameraComponent.h>

namespace brr::vis
{
    class Window;

    class SceneView
    {
    public:

        SceneView(Window* window);

        SceneView(Window* window, const PerspectiveCameraComponent* camera);

        ~SceneView();

        // Return the current assigned Camera. If no Camera is assigned, return nullptr.
        PerspectiveCameraComponent* GetCameraComponent() const;

        // Set the current Camera. Pass nullptr to reset Camera.
        void SetCamera(const PerspectiveCameraComponent* camera);

        // UnsetCamera Camera from SceneView.
        void UnsetCamera();

    private:

        // Callback function called when current assigned camera is destroyed.
        void OnCameraDestroyed();

        std::shared_ptr<EventAction<>> m_camera_destroyed_action;
        Window* m_window;
        Scene* m_scene;
        Entity m_camera_entity;
    };
}

#endif
