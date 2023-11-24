#include "maths.hpp"

#ifdef GRAPHICS_VULKAN
glm::vec2 aglMath::ConvertExtents(VkExtent2D extent)
{
	return glm::vec2(extent.width, extent.height);
}
#endif
