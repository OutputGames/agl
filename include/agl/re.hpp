#ifndef RE_HPP
#define RE_HPP

#define NOMINMAX

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "spirv_reflect/spirv_reflect.h"
#define vkDestroy(F,T) DestroyVulkanObject<decltype(T)>(F, T)
#include "SDL_vulkan.h"

#include "aurora/utils/utils.hpp"

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

#endif
