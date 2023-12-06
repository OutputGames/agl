#ifndef defined(EXT_HPP)
#define EXT_HPP

#include "re.hpp"

struct Light
{
	alignas(16) vec3 position;
	alignas(16) vec3 color;
	alignas(4) float power;
};

struct LightingSettings
{
	alignas(32) Light lights[4];

	alignas(16) vec3 albedo;

	alignas(4) int lightAmount;
	alignas(4) float metallic;
	alignas(4) float roughness;
};

#endif // EXT_HPP
