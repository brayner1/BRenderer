#include "Core/PerspectiveCamera.h"

brr::PerspectiveCamera::PerspectiveCamera(glm::vec3 position, glm::vec3 target, float fovy, float aspect, float near, float far)
{
	SetViewMatrix(position, target, { 0.f, -1.f, 0.f });

	projection_matrix_ = glm::perspective(fovy, aspect, near, far);
}

void brr::PerspectiveCamera::SetViewMatrix(glm::vec3 position, glm::vec3 target, glm::vec3 up)
{
	view_matrix_ = glm::lookAt(position, target, up);
}
