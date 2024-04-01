#version 450

#pragma shader_stage(fragment)

layout(set = 0, binding = 0, std140) uniform DummyUniform {
    vec4 dummy[25];
};

layout(push_constant) uniform PushConstants {
    vec2 resolution;
};

layout(location = 0) out vec4 glFragColor;

void main() {
    vec2 uv = ((resolution - gl_FragCoord.xy) / resolution);
    glFragColor = vec4(1.0 - uv.x, uv.y, dummy[10].w, 1.0);
}