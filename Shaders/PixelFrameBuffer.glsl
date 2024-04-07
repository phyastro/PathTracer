#version 460 core

layout(location = 0) out vec4 FragColor;

uniform ivec2 resolution;
uniform int tonemap;
uniform sampler2D screenTexture;

// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
vec3 XYZToRGB(vec3 XYZ){
	// Conversion From Illuminant E XYZ To CIE RGB Color Space
	mat3 m = mat3(2.3706743, -0.9000405, -0.4706338, -0.5138850, 1.4253036, 0.0885814, 0.0052982, -0.0146949, 1.0093968);
	return XYZ * m;
}

vec3 Gamma(vec3 x, float y) {
	// Gamma Function x^(1/y)
	x = clamp(x, 0.0, 1.0);
	return pow(x, 1.0 / vec3(y));
}

vec3 Reinhard(vec3 x) {
	// x / (1 + x)
	return x / (1.0 + x);
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x) {
	// x(ax + b) / (x(cx + d) + e)
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return x * (a * x + b) / (x * (c * x + d) + e);
}

// DEUCES Biophotometric Tonemap by Ted(Kerdek)
vec3 DEUCESBioPhotometric(vec3 x) {
	// e^(-0.25 / x)
	return exp(-0.25 / x);
}

void main() {
	vec3 color = vec3(0.0);
	color = texture(screenTexture, gl_FragCoord.xy / resolution).xyz;
	color = max(XYZToRGB(color), 0.0);
	if (tonemap == 1)
		color = Reinhard(color);
	if (tonemap == 2)
		color = ACESFilm(color);
	if (tonemap == 3)
		color = DEUCESBioPhotometric(color);
	color = Gamma(color, 2.2);
	FragColor = vec4(color, 1.0);
}