#version 450

#pragma shader_stage(fragment)

layout(set = 0, binding = 1) uniform sampler2D rendering;

layout(push_constant) uniform PushConstants {
    vec2 resolution;
};

layout(location = 0) out vec4 glFragColor;

void main() {
    glFragColor = texelFetch(rendering, ivec2(gl_FragCoord.xy), 0);
}