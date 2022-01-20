#version 450
/*
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
*/
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in vec3 worldNormal;
layout(location = 4) in vec3 viewPos;
layout(location = 5) in vec3 diffuseColor;
layout(location = 6) in vec3 specularColor;
layout(location = 7) in float gloss;

// 方向光
layout(location = 8) in vec3 directLightColor;
layout(location = 9) in vec3 directLightDir;
// 点光源
layout(location = 10) in vec3 pointLightColor;
layout(location = 11) in vec3 pointLightPos;
// 聚光灯
layout(location = 12) in vec3 spotLightColor;
layout(location = 13) in vec3 spotLightPos;
layout(location = 14) in vec3 spotLightDir;
layout(location = 15) in float spotLightPhi;

layout(location = 0) out vec4 outColor;

vec3 Phong(vec3 lightColor, vec3 lightDir, vec3 viewDir, vec3 normal, vec3 texColor){
    // 环境光
    vec3 ambient = lightColor * 0.1;
    // 漫反射
    float diffuseFrac = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diffuseFrac * lightColor * diffuseColor;
    // 高光
    vec3 reflectDir = 2 * dot(normal, lightDir) * normal - lightDir;
    float specularFrac = pow(max(dot(viewDir, reflectDir), 0.0), gloss);
    vec3 specular = specularFrac * lightColor * specularColor; 

    return (ambient + diffuse + specular) * texColor;
}

// 方向光
vec3 DirectLight(vec3 lightColor, vec3 lightDir, vec3 viewDir, vec3 normal, vec3 texColor){
    return Phong(lightColor, -lightDir, viewDir, normal, texColor);
}

// 点光源
vec3 PointLight(vec3 lightColor, vec3 lightPos, vec3 worldPos, vec3 viewDir, vec3 normal, vec3 texColor){
    vec3 lightDir = normalize(lightPos - worldPos); 
    // 衰减
    float constant = 1.0;
    float linear = 0.7;
    float quadratic = 1.8;
    float dis = length(lightPos - worldPos);
    float attenuation = 1.0 / (constant + linear * dis + quadratic * dis * dis);
    return attenuation * Phong(lightColor, lightDir, viewDir, normal, texColor);
}

// 聚光灯
vec3 SpotLight(vec3 lightColor, vec3 lightPos, vec3 spotDir, float cosPhi, vec3 worldPos, vec3 viewDir, vec3 normal, vec3 texColor){
    vec3 lightDir = normalize(lightPos - worldPos);
    float cosTheta = dot(lightDir, spotDir);
    float intensity = clamp((cosTheta - cosPhi) / (cosPhi - cosPhi * 0.9), 0.0, 1.0);
    return intensity * Phong(lightColor, lightDir, viewDir, normal, texColor);
}

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 viewDir = normalize(viewPos - worldPos);
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;
    // vec3 direct = DirectLight(ubo.directLightColor, ubo.directLightDir, viewDir, normal, texColor);
    // vec3 point = PointLight(ubo.pointLightColor, ubo.pointLightPos, worldPos, viewDir, normal, texColor);
    // vec3 spot = SpotLight(ubo.spotLightColor, ubo.spotLightPos, ubo.spotLightDir, ubo.spotLightPhi, worldPos, viewDir, normal, texColor);
    vec3 direct = DirectLight(directLightColor, directLightDir, viewDir, normal, texColor);
    vec3 point = PointLight(pointLightColor, pointLightPos, worldPos, viewDir, normal, texColor);
    vec3 spot = SpotLight(spotLightColor, spotLightPos, spotLightDir, spotLightPhi, worldPos, viewDir, normal, texColor);

    outColor = vec4(direct + point + spot, 1.0);
}
/*
void main(){
    //Ambient Lighting
    vec3 ambient = 0.1 * pointLightColor;

	//Diffuse Lighting
	vec3 norm = normalize(worldNormal);
    vec3 lightDir = normalize(pointLightPos - worldPos);
	float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * pointLightColor * diffuseColor;

	//Specular Lighting
	vec3 viewDir = normalize(viewPos - worldPos);
    vec3 reflectDir = 2 * dot(norm, lightDir) * norm - lightDir;
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), gloss);
    vec3 specular = spec * pointLightColor * specularColor;

    vec3 texColor = texture(texSampler, fragTexCoord).rgb; 

    float constant = 1.0;
    float linear = 0.7;
    float quadratic = 1.8;
    float dis = length(pointLightPos - worldPos);
    float attenuation = 1.0 / (constant + linear * dis + quadratic * dis * dis);
    vec3 result = (ambient + diffuse + specular) * texColor * attenuation;
    outColor = vec4(result, 1.0);
}
*/