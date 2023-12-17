#ifndef RE_HPP
#define RE_HPP

#ifdef GRAPHICS_OPENGL
#include <GL/glew.h>
#endif

#define NOMINMAX

#ifdef GRAPHICS_VULKAN

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "spirv_reflect/spirv_reflect.h"
#define vkDestroy(F,T) DestroyVulkanObject<decltype(T)>(F, T)
#include "SDL_vulkan.h"

#endif

#include "aurora/utils/utils.hpp"


#define cast(O,T) static_cast<T>(O)

#define loop(c, cl, vn) alignas(sizeof(cl)) cl vn##c;

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

#endif
