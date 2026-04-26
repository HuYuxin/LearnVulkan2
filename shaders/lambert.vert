#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 fragTBN;
layout(location = 5) out vec4 fragPositionLightSpace;

layout(push_constant) uniform PushConsts {
	mat4 model;
} primitive;

void main() {
    mat4 modelMatrix = ubo.model * primitive.model;
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

    vec3 T = normalize(normalMatrix * inTangent.xyz);
    vec3 N = normalize(normalMatrix * inNormal);
    vec3 B = cross(N, T) * inTangent.w;

    fragTBN = mat3(T, B, N);

    gl_Position = ubo.proj * ubo.view * modelMatrix * vec4(inPosition, 1.0);
    vec4 modelPosWorldSpace = modelMatrix * vec4(inPosition, 1.0);
    fragPosition = vec3(modelPosWorldSpace.x/modelPosWorldSpace.w, modelPosWorldSpace.y/modelPosWorldSpace.w, modelPosWorldSpace.z/modelPosWorldSpace.w);
    fragTexCoord = inTexCoord;
    fragPositionLightSpace = ubo.lightSpaceMatrix * modelMatrix * vec4(inPosition, 1.0);
}