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

        /**
         * Transform the camera to look at a target from a specified position and an up vector.
         * @param eye Camera eye position.
         * @param target Camera target point (to calculate orientation)
         * @param up Camera up vector.
         */
        void LookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up);

        /**
         * @return Camera view matrix.
         */
        [[nodiscard]] glm::mat4 GetViewMatrix() const;

        /**
         * @param aspect_ratio aspect ratio used to calculate camera projection.
         * @return Camera projection matrix given an aspect ratio.
         */
        [[nodiscard]] glm::mat4 GetProjectionMatrix(float aspect_ratio) const;

        /**
         * @param aspect_ratio aspect ratio used to calculate camera projection.
         * @return Camera projection-view matrix given an aspect ratio.
         */
        [[nodiscard]] glm::mat4 GetProjectionViewMatrix(float aspect_ratio) const;

        /**
         * Project a point to viewport coordinate system.
         * X and Y values from returned point ranges from (-1, -1) to (1, 1), where (-1 -1) is viewport's top-left and (1, 1) is viewport's bottom-right.
         * The Z coordinate is the distance from the camera in world units.
         * @param point Point to transform to viewport coordinates.
         * @param aspect Aspect ratio of the viewport.
         * @return Point projected on viewport coordinates.
         */
        [[nodiscard]] glm::vec3 TransformToViewportCoords(const glm::vec3& point, float aspect) const;


        /**
         * @return Camera render ID in RenderThread. Will return CameraID::NULL_ID if SceneRenderer is not initialized.
         */
        [[nodiscard]] render::CameraID GetCameraRenderID() const { return m_camera_id; }

    public:
        /****************************
         * Entity Component Methods *
         ****************************/

        void OnInit();
        void OnDestroy();

        void RegisterGraphics();
		void UnregisterGraphics();

	private:

		float m_fov_y;
		float m_near;
		float m_far;

        render::CameraID m_camera_id;
    };
}

#endif
