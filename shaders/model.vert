#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    vec3 viewPos;
    vec3 diffuseColor;
    vec3 specularColor;
    float gloss;
    // 方向光
    vec3 directLightColor;
    vec3 directLightDir;
    // 点光源
    vec3 pointLightColor;
    vec3 pointLightPos;
    // 聚光灯
    vec3 spotLightColor;
    vec3 spotLightPos;
    vec3 spotLightDir;
    float spotLightPhi;

} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 worldPos;
layout(location = 3) out vec3 worldNormal;
layout(location = 4) out vec3 viewPos;
layout(location = 5) out vec3 diffuseColor;
layout(location = 6) out vec3 specularColor;
layout(location = 7) out float gloss;

// 方向光
layout(location = 8) out vec3 directLightColor;
layout(location = 9) out vec3 directLightDir;
// 点光源
layout(location = 10) out vec3 pointLightColor;
layout(location = 11) out vec3 pointLightPos;
// 聚光灯
layout(location = 12) out vec3 spotLightColor;
layout(location = 13) out vec3 spotLightPos;
layout(location = 14) out vec3 spotLightDir;
layout(location = 15) out float spotLightPhi;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    worldPos = vec3(ubo.model * vec4(inPosition, 1.0));
    worldNormal = mat3(transpose(inverse(ubo.model))) * normal;

    viewPos = ubo.viewPos;
    
    diffuseColor = ubo.diffuseColor;
    specularColor = ubo.specularColor;
    gloss = ubo.gloss;

    // 方向光
    directLightColor = ubo.directLightColor;
    directLightDir = ubo.directLightDir;
    // 点光源
    pointLightColor = ubo.pointLightColor;
    pointLightPos = ubo.pointLightPos;
    // 聚光灯
    spotLightColor = ubo.spotLightColor;
    spotLightPos = ubo.spotLightPos;
    spotLightDir = ubo.spotLightDir;
    spotLightPhi = ubo.spotLightPhi;


}