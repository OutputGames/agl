#ifndef RE_HPP
#define RE_HPP

#include <string>
#include <iostream>
#include  <vector>
#include <map>
#include <optional>
#include <set>
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <fstream>
#include <sstream>
#include <chrono>
#include <utility>
#include <filesystem>

#ifdef GRAPHICS_OPENGL
#include <GL/glew.h>
#endif

#define NOMINMAX

#ifdef GRAPHICS_VULKAN

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include "spirv_reflect/spirv_reflect.h"
#define vkDestroy(F,T) DestroyVulkanObject<decltype(T)>(F, T)

#endif



#include <iostream>
#include <stdexcept>
#include <cstdlib>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>

using namespace glm;

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


#define flt (float)

#define cast(O,T) static_cast<T>(O)

#define loop(c, cl, vn) alignas(sizeof(cl)) cl vn##c;


typedef uint32_t u32;
typedef uint16_t u16;
typedef const char cc;
typedef size_t szt;

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

#endif
