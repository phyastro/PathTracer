#version 450

#pragma shader_stage(fragment)

layout(location = 0) out vec4 glFragColor;

const vec2 resolution = vec2(800.0, 600.0);

void main() {
    vec2 uv = ((resolution - gl_FragCoord.xy) / resolution);
    glFragColor = vec4(1.0 - uv.x, uv.y, 0.0, 1.0);
}