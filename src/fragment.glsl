#version 450

#pragma shader_stage(fragment)

layout(location = 0) in vec3 outColor;
layout(location = 0) out vec4 glFragColor;

void main() {
    glFragColor = vec4(outColor, 1.0);
}