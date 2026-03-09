#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragPositionLightSpace;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
} ubo;
layout(set = 1, binding = 0) uniform sampler2D shadowMap;
layout(set = 2, binding = 0) uniform sampler2D texSampler;

float calculateShadowFactor(vec4 fragmentPositionLightSpace) {
    vec3 lightSpaceProjectionCoords = fragmentPositionLightSpace.xyz / fragmentPositionLightSpace.w;
    lightSpaceProjectionCoords.xy = lightSpaceProjectionCoords.xy * 0.5 + 0.5;
    float closestDepthToLight = texture(shadowMap, lightSpaceProjectionCoords.xy).r;
    float currentDepth = lightSpaceProjectionCoords.z;
    return currentDepth > (closestDepthToLight + 0.1) ? 1.0 : 0.0;
}

void main() {
    vec3 lightDirection = normalize(ubo.lightPos - fragPosition);
    float lambertFactor = max(dot(normalize(fragNormal), lightDirection), 0);
    float ambient = 0.1;
    float shadow = calculateShadowFactor(fragPositionLightSpace);
    outColor = (ambient + (1.0 - shadow) * lambertFactor) * texture(texSampler, fragTexCoord);
}
