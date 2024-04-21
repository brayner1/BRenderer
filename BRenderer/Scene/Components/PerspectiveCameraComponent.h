#ifndef BRR_PERSPECTIVECAMERACOMPONENT_H
#define BRR_PERSPECTIVECAMERACOMPONENT_H
#include <Scene/Components/EntityComponent.h>

#include <Renderer/SceneObjectsIDs.h>

namespace brr
{
    class PerspectiveCameraComponent : public EntityComponent
    {
    public:
        PerspectiveCameraComponent(float fovy, float near, float far);


        [[nodiscard]] render::CameraId GetCameraRenderID() const { return m_camera_id; }

		//[[nodiscard]] const glm::mat4& GetViewMatrix() const { return view_matrix_; }
        void OnInit();
        void RegisterGraphics();
		void UnregisterGraphics();

	private:

		float m_fov_y;
		float m_near;
		float m_far;

        render::CameraId m_camera_id;
    };
}

#endif
