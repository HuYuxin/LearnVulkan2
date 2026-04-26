#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 fragTBN;
layout(location = 5) in vec4 fragPositionLightSpace;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 viewDir;
} ubo;
layout(set = 1, binding = 0) uniform sampler2D shadowMap;
layout(set = 2, binding = 0) uniform sampler2D baseColorTexSampler;
layout(set = 2, binding = 1) uniform sampler2D metallicRoughnessTexSampler;
layout(set = 2, binding = 2) uniform sampler2D normalTexSampler;
layout(set = 3, binding = 0) uniform DirLightUBO {
    vec3 direction;
    float padding1;
    vec3 ambient;
    float padding2;
    vec3 diffuse;
    float padding3;
    vec3 specular;
    float padding4;
} dirLight;

float calculateShadowFactor(vec4 fragmentPositionLightSpace) {
    vec3 lightSpaceProjectionCoords = fragmentPositionLightSpace.xyz / fragmentPositionLightSpace.w;
    lightSpaceProjectionCoords.xy = lightSpaceProjectionCoords.xy * 0.5 + 0.5;
    float closestDepthToLight = texture(shadowMap, lightSpaceProjectionCoords.xy).r;
    float currentDepth = lightSpaceProjectionCoords.z;
    return currentDepth > (closestDepthToLight + 0.1) ? 1.0 : 0.0;
}

vec3 getDirectionalLight(vec3 viewDir)
{
    vec3 normal = texture(normalTexSampler, fragTexCoord).rgb * 2.0 - 1.0;
    vec3 worldSpaceNormal = normalize(fragTBN * normal);
    vec3 lightDir = normalize(-dirLight.direction);
    float lambertFactor = max(dot(worldSpaceNormal, lightDir), 0.0);
    vec3 reflectionDir = reflect(-lightDir, worldSpaceNormal);

    vec3 baseColor = texture(baseColorTexSampler, fragTexCoord).rgb;
    float metallic = texture(metallicRoughnessTexSampler, fragTexCoord).b;
    float roughness = texture(metallicRoughnessTexSampler, fragTexCoord).g;

    float shiness = mix(256.0, 2.0, roughness);
    vec3 fresnel = mix(vec3(0.04), baseColor, metallic);
    float specularFactor = pow(max(dot(viewDir, reflectionDir), 0.0), shiness);

    vec3 diffuseColor = baseColor * (1.0 - metallic);
    vec3 ambient = dirLight.ambient * baseColor;
    vec3 diffuse = dirLight.diffuse * lambertFactor * diffuseColor;
    vec3 specular = dirLight.specular * specularFactor * fresnel;

    return ambient + diffuse + specular;
}

void main() {
    outColor = vec4(getDirectionalLight(normalize(ubo.viewDir)), 1.0);
}
