#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPos;
layout(binding = 1) uniform sampler2D albedo;
layout(binding = 2) uniform sampler2D normalMap;

layout(location = 0) out vec4 outColor;

vec3 gamma(vec3 v) {
    float gamma = 2.2;
    v.rgb = pow(v.rgb, vec3(1.0/gamma));

    return v;
}

vec3 getNormalFromMap()
{
    
    vec3 normal = texture(normalMap, fragTexCoord).xyz;

    //normal.g = 1.0;

    normal = gamma(normal);

    vec3 tangentNormal = normal * 2.0 - 1.0;



    vec3 Q1  = dFdx(fragPos);
    vec3 Q2  = dFdy(fragPos);
    vec2 st1 = dFdx(fragTexCoord);
    vec2 st2 = dFdy(fragTexCoord);

    vec3 N   = normalize(fragNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {

    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightPos = vec3(10,10,10);

    vec3 normalM = getNormalFromMap();

    vec3 norm = normalM;
    vec3 lightDir = normalize(lightPos - fragPos);

    float diff = max(dot(norm,lightDir),0.0);
    vec3 diffuse = diff * lightColor;


    outColor = texture(albedo, fragTexCoord);

    vec3 result = (ambient + diffuse) * outColor.rgb;

    outColor.rgb = result;

    outColor.rgb = gamma(outColor.rgb);

    //outColor.rgb = normalM;
}