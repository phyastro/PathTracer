#version 450

#pragma shader_stage(fragment)

layout(set = 0, binding = 1, rgba32f) uniform readonly imageBuffer texelBuffer;

layout(push_constant) uniform PushConstants {
    ivec2 resolution;
	int frame;
    int samples;
    int samplesPerFrame;
    float FPS;
    float persistence;
    int pathLength;
    vec2 cameraAngle;
    float cameraPosX;
	float cameraPosY;
	float cameraPosZ;
    int ISO;
    float cameraSize;
    float apertureSize;
    float apertureDist;
	float lensRadius;
	float lensFocalLength;
	float lensThickness;
	float lensDistance;
    int tonemap;
};

layout(location = 0) out vec4 processorColor;

// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
vec3 XYZToRGB(vec3 XYZ){
	// Conversion From Illuminant E XYZ To CIE RGB Color Space
	mat3 m = mat3(2.3706743, -0.9000405, -0.4706338, -0.5138850, 1.4253036, 0.0885814, 0.0052982, -0.0146949, 1.0093968);
	return XYZ * m;
}

vec3 Gamma(in vec3 x, in float y) {
	// Gamma Function x^(1/y)
	x = clamp(x, 0.0, 1.0);
	return pow(x, 1.0 / vec3(y));
}

vec3 Reinhard(in vec3 x) {
	// x / (1 + x)
	return x / (1.0 + x);
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(in vec3 x) {
	// x(ax + b) / (x(cx + d) + e)
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return x * (a * x + b) / (x * (c * x + d) + e);
}

// DEUCES Biophotometric Tonemap by Ted(Kerdek)
vec3 DEUCESBioPhotometric(in vec3 x) {
	// e^(-0.25 / x)
	return exp(-0.25 / x);
}

vec3 Processing(in vec3 inColor) {
	vec3 outColor = max(XYZToRGB(inColor), 0.0);
	if (tonemap == 1)
		outColor = Reinhard(outColor);
	if (tonemap == 2)
		outColor = ACESFilm(outColor);
	if (tonemap == 3)
		outColor = DEUCESBioPhotometric(outColor);
	outColor = Gamma(outColor, 2.2);

	return outColor;
}

void main() {
	int coords = int(gl_FragCoord.x) + resolution.x * int(gl_FragCoord.y);
	vec3 rendererColor = imageLoad(texelBuffer, coords).xyz;
	processorColor = vec4(Processing(rendererColor), 1.0);
}