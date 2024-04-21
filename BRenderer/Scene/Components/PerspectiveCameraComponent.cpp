#include "PerspectiveCameraComponent.h"

#include <Scene/Components/Transform3DComponent.h>

namespace brr
{
    PerspectiveCameraComponent::PerspectiveCameraComponent(float fovy, float near, float far)
    : m_fov_y(fovy),
	  m_near(near),
	  m_far(far),
	  m_camera_id(render::CameraId::NULL_ID)
	{}

    void PerspectiveCameraComponent::OnInit()
    {
        if (GetScene()->GetMainCamera() == nullptr)
        {
            GetScene()->SetMainCamera(GetEntity());
        }
    }

    void PerspectiveCameraComponent::RegisterGraphics()
    {
        vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_render_proxy && "Can't call 'PerspectiveCameraComponent::RegisterGraphics' when SceneRenderer is NULL.");
        Transform3DComponent& owner_transform = GetEntity().GetComponent<Transform3DComponent>();
		m_camera_id = scene_render_proxy->CreateCamera(owner_transform, m_fov_y, m_near, m_far);
    }

    void PerspectiveCameraComponent::UnregisterGraphics()
    {
        vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_render_proxy && "Can't call 'PerspectiveCameraComponent::UnregisterGraphics' when SceneRenderer is NULL.");
		scene_render_proxy->DestroyCamera(m_camera_id);
        m_camera_id = render::CameraId::NULL_ID;
    }
}
