#version 450

#pragma shader_stage(fragment)

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec2 resolution;
} ubo;

layout(location = 0) out vec4 glFragColor;

void main() {
    vec2 uv = ((ubo.resolution - gl_FragCoord.xy) / ubo.resolution);
    glFragColor = vec4(1.0 - uv.x, uv.y, 0.0, 1.0);
}