#version 450

#pragma shader_stage(fragment)

layout(set = 0, binding = 0, std140) uniform DummyUniform {
    vec4 dummy[25];
};

layout(set = 0, binding = 1) uniform sampler2D prevRendererImage;

layout(push_constant) uniform PushConstants {
    vec2 resolution;
    int frame;
};

layout(location = 0) out vec4 rendererColor;
layout(location = 1) out vec4 processorColor;

void main() {
	vec2 uv = ((resolution - gl_FragCoord.xy) / resolution);
    vec4 prevRendererColor = texelFetch(prevRendererImage, ivec2(gl_FragCoord.xy), 0);
	rendererColor = ((frame - 1) * prevRendererColor + vec4(1.0 - uv.x, uv.y, dummy[10].w, 1.0)) / frame;
    processorColor = rendererColor;
}