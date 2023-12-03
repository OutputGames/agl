#ifndef defined(EXT_HPP)
#define EXT_HPP

#include "re.hpp"

struct Camera
{
	vec3 position={0,0,2};
	vec3 target;
	vec3 up = { 0.0,1.0,0.0 };
	vec3 right = { 1.0,0,0 };
	float fov = 45.0f;

	void Update(vec2 extents);

	mat4 GetViewMatrix()
	{
		return view;
	}

	mat4 GetProjectionMatrix()
	{
		return proj;
	}

private:
	mat4 view=mat4(1.0), proj=mat4(1.0);
};

struct UniformBufferObject
{
	alignas(16) mat4 model;
	alignas(16) mat4 view;
	alignas(16) mat4 proj;
	alignas(16) mat3 normalMatrix;
	alignas(16) vec3 camPos;
};

struct Light
{
	alignas(16) vec3 position;
	alignas(16) vec3 color;
	alignas(4) float power;
};

struct LightingSettings
{
	alignas(32) Light lights[4];

	loop(0, Light, light)

	alignas(16) vec3 albedo;

	alignas(4) int lightAmount;
	alignas(4) float metallic;
	alignas(4) float roughness;
};

#endif // EXT_HPP
