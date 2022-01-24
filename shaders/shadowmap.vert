#version 450
 
layout(location = 0) in vec3 inPosition;

layout(binding = 0)uniform ShadowPassUbo {
    mat4 mvp;
}ubo;
 
void main(){
   gl_Position = ubo.mvp * vec4(inPosition, 1.0);
}