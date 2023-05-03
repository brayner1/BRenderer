#ifndef VKR_PCH_H
#define VKR_PCH_H

#define NOMINMAX
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include <chrono>
#include <limits>
#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <utility>
#include <optional>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "vulkan/vulkan.hpp"

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/matrix_decompose.hpp"

#include "entt/entity/registry.hpp"

#undef near
#undef far

#endif