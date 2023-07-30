#ifndef VKR_ThirdpartiesInc_H
#define VKR_ThirdpartiesInc_H

#define NOMINMAX

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/matrix_decompose.hpp"

#include "entt/entity/registry.hpp"

#undef near
#undef far

#endif