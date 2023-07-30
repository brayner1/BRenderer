#ifndef BRR_PERSPECTIVECAMERA_H
#define BRR_PERSPECTIVECAMERA_H

#include <Core/thirdpartiesInc.h>

namespace brr
{
	class PerspectiveCamera
	{
	public:

		PerspectiveCamera(glm::vec3 position, glm::vec3 target, float fovy, float aspect, float near, float far);

		[[nodiscard]] const glm::mat4& GetViewMatrix() const { return view_matrix_; }
		[[nodiscard]] const glm::mat4& GetProjectionMatrix() const { return projection_matrix_; }

		[[nodiscard]] glm::mat4 GetProjectionViewMatrix() const { return projection_matrix_ * view_matrix_; }

	private:
		void SetViewMatrix(glm::vec3 position, glm::vec3 target, glm::vec3 up);

		glm::mat4 view_matrix_ {1.f};
		glm::mat4 projection_matrix_ {1.f};
	};
}

#endif