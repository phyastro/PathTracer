#version 460 core

layout(location = 0) out vec4 FragColor;

uniform ivec2 resolution;
uniform int frame;
uniform int sFrame;
uniform sampler2D screenTexture;

vec3 XYZToRGB(vec3 XYZ){
	// Conversion From XYZ To RGB Color Space
	mat3 m = mat3(2.3706743, -0.9000405, -0.4706338, -0.5138850, 1.4253036, 0.0885814, 0.0052982, -0.0146949, 1.0093968);
	return XYZ * m;
}

vec3 Gamma(vec3 x, float y) {
	// Gamma Function (x^(1/y))
	x = clamp(x, 0.0, 1.0);
	return pow(x, 1.0 / vec3(y));
}

vec3 BioPhotometricTonemapping(vec3 x) {
	// Biophotometric Tonemapping by Ted
	return exp(-1.0 / x);
}

void main() {
	vec3 color = vec3(0.0);
	color = texture(screenTexture, gl_FragCoord.xy / resolution).xyz / sFrame;
	color = Gamma(BioPhotometricTonemapping(max(XYZToRGB(color), 0.0)), 2.2);
	FragColor = vec4(color, 1.0);
}