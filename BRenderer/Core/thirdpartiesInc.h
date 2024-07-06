#ifndef VKR_ThirdpartiesInc_H
#define VKR_ThirdpartiesInc_H

#include "SDL2/SDL.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"

#include "entt/entity/registry.hpp"

#undef near
#undef far

#endif