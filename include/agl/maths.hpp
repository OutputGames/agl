#if !defined(MATHS_HPP)
#define MATHS_HPP

	#include "re.hpp"

struct aglMath {

#ifdef GRAPHICS_VULKAN
	static glm::vec2 ConvertExtents(VkExtent2D extent);
#endif
};

#endif // MATHS_HPP