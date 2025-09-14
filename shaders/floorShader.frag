#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
} ubo;

void main() {
    vec3 lightDirection = normalize(ubo.lightPos - fragPosition);
    float lambertFactor = max(dot(normalize(fragNormal), lightDirection), 0);
    float ambient = 0.2;
    outColor = (ambient + lambertFactor) * vec4(1.0, 1.0, 1.0, 1.0);
}
