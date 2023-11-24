#include "ext.hpp"



void Camera::Update(vec2 extent)
{
	vec3 direction = normalize(position - target);

	right = normalize(cross(up, direction));
	up = cross(direction, right);

	view = lookAt(position, target, { 0,1,0 });

	proj = glm::perspective(glm::radians(fov), flt extent.x / flt extent.y, 0.1f, 100.0f);

}