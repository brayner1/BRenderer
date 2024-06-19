#include "PerspectiveCameraComponent.h"

#include <Scene/Components/Transform3DComponent.h>

namespace brr
{
    PerspectiveCameraComponent::PerspectiveCameraComponent(float fov_y, float near, float far)
    : m_fov_y(fov_y),
	  m_near(near),
	  m_far(far),
	  m_camera_id(render::CameraID::NULL_ID)
	{}

    void PerspectiveCameraComponent::LookAt(const glm::vec3& eye,
                                            const glm::vec3& target,
                                            const glm::vec3& up)
    {
        glm::vec3 look_dir = glm::normalize(target - eye);

        Transform3DComponent& transform = GetEntity().GetComponent<Transform3DComponent>();
        transform.SetPosition(eye);
        transform.SetRotation(glm::quatLookAt(look_dir, up));
    }

    glm::mat4 PerspectiveCameraComponent::GetViewMatrix() const
    {
        Transform3DComponent& transform = GetEntity().GetComponent<Transform3DComponent>();
        return glm::inverse(transform.GetGlobalMatrix());
    }

    glm::mat4 PerspectiveCameraComponent::GetProjectionMatrix(float aspect_ratio) const
    {
        return glm::perspective(m_fov_y, aspect_ratio, m_near, m_far);
    }

    glm::mat4 PerspectiveCameraComponent::GetProjectionViewMatrix(float aspect_ratio) const
    {
        return GetProjectionMatrix(aspect_ratio) * GetViewMatrix();
    }

    glm::vec3 PerspectiveCameraComponent::TransformToViewportCoords(const glm::vec3& point,
                                                                    float aspect) const
    {
        glm::vec4 point_vec = {point, 1.0};
        glm::mat4 view_matrix = GetViewMatrix();
        glm::mat4 proj_matrix = GetProjectionMatrix(aspect);

        glm::vec4 point_view_coords = view_matrix * point_vec;
        glm::vec4 point_ndc_coords = proj_matrix * point_view_coords;
        return {point_ndc_coords.x / point_ndc_coords.w, point_ndc_coords.y / point_ndc_coords.w, point_view_coords.z};
    }

    void PerspectiveCameraComponent::SetFovY(float fov_y)
    {
        if (m_fov_y != fov_y)
        {
            m_fov_y = fov_y;
            vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
            scene_render_proxy->UpdateCameraProjectionMatrix(m_camera_id, m_fov_y, m_near, m_far);
        }
    }

    void PerspectiveCameraComponent::SetNear(float near)
    {
        if (m_near != near)
        {
            m_near = near;
            vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
            scene_render_proxy->UpdateCameraProjectionMatrix(m_camera_id, m_fov_y, m_near, m_far);
        }
    }

    void PerspectiveCameraComponent::SetFar(float far)
    {
        if (m_far != far)
        {
            m_far = far;
            vis::SceneRenderProxy* scene_render_proxy = GetScene()->GetSceneRendererProxy();
            scene_render_proxy->UpdateCameraProjectionMatrix(m_camera_id, m_fov_y, m_near, m_far);
        }
    }

    void PerspectiveCameraComponent::OnInit()
    {
        if (GetScene()->GetMainCamera() == nullptr)
        {
            GetScene()->SetMainCamera(GetEntity());
        }
    }

    void PerspectiveCameraComponent::OnDestroy()
    {
        if (GetScene()->GetMainCamera() == this)
        {
            //GetScene()->SetMainCamera(nullptr);
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
        m_camera_id = render::CameraID::NULL_ID;
    }
}
