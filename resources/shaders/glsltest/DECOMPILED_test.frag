#version 330
#ifdef GL_ARB_shading_language_420pack
#extension GL_ARB_shading_language_420pack : require
#endif

layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 1) uniform sampler2D albedo;

in vec2 fragTexCoord;
in vec3 fragPos;
in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

vec3 gamma(inout vec3 v)
{
    float gamma_1 = 2.2000000476837158203125;
    v = pow(v, vec3(1.0 / gamma_1));
    return v;
}

vec3 getNormalFromMap()
{
    vec3 normal = texture(normalMap, fragTexCoord).xyz;
    vec3 param = normal;
    vec3 _43 = gamma(param);
    normal = _43;
    vec3 tangentNormal = (normal * 2.0) - vec3(1.0);
    vec3 Q1 = dFdx(fragPos);
    vec3 Q2 = dFdy(fragPos);
    vec2 st1 = dFdx(fragTexCoord);
    vec2 st2 = dFdy(fragTexCoord);
    vec3 N = normalize(fragNormal);
    vec3 T = normalize((Q1 * st2.y) - (Q2 * st1.y));
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(vec3(T), vec3(B), vec3(N));
    return normalize(TBN * tangentNormal);
}

void main()
{
    vec3 lightColor = vec3(1.0);
    float ambientStrength = 0.100000001490116119384765625;
    vec3 ambient = lightColor * ambientStrength;
    vec3 lightPos = vec3(10.0);
    vec3 normalM = getNormalFromMap();
    vec3 norm = normalM;
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * diff;
    outColor = texture(albedo, fragTexCoord);
    vec3 result = (ambient + diffuse) * outColor.xyz;
    outColor.x = result.x;
    outColor.y = result.y;
    outColor.z = result.z;
    vec3 param = outColor.xyz;
    vec3 _169 = gamma(param);
    outColor.x = _169.x;
    outColor.y = _169.y;
    outColor.z = _169.z;
}

