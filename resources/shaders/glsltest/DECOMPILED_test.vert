#version 330
#ifdef GL_ARB_shading_language_420pack
#extension GL_ARB_shading_language_420pack : require
#endif

layout(binding = 0, std140) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 camPos;
} ubo;

layout(location = 0) in vec3 inPosition;
out vec3 fragNormal;
layout(location = 1) in vec3 inNormal;
out vec2 fragTexCoord;
layout(location = 2) in vec2 inTexCoord;
out vec3 fragPos;

void main()
{
    gl_Position = ((ubo.proj * ubo.view) * ubo.model) * vec4(inPosition, 1.0);
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
    fragPos = inPosition;
}

