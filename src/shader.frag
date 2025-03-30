#version 450

layout(set = 0, binding = 1, rgba32f) uniform readonly imageBuffer texelBuffer;

layout(push_constant) uniform PushConstants {
    ivec2 resolution;
    int frame;
    int currentSamples;
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

// http://www.brucelindbloom.com/Eqn_ChromAdapt.html
vec3 IlluminantEToD65(vec3 XYZ) {
    // Bradford Chromatic Adaptation From Reference White Illuminant E To Illuminant D65
    mat3 m = mat3(0.9531874, -0.0265906, 0.0238731,
            -0.0382467, 1.0288406, 0.0094060,
            0.0026068, -0.0030332, 1.0892565);
    return XYZ * m;
}

// http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html
vec3 XYZToRGB(vec3 XYZ) {
    // Transformation From XYZ To sRGB Color Space With Illuminant D65 As Reference White
    mat3 m = mat3(3.2404542, -1.5371385, -0.4985314,
            -0.9692660, 1.8760108, 0.0415560,
            0.0556434, -0.2040259, 1.0572252);
    return XYZ * m;
}

float sRGBCompanding(in float x) {
    // Companding For sRGB Displays
    x = clamp(x, 0.0, 1.0);
    if (x <= 0.0031308) {
        x = 12.92 * x;
    } else {
        x = 1.055 * pow(x, 1.0 / 2.4) - 0.055;
    }
    return x;
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
    inColor = IlluminantEToD65(inColor);
    vec3 outColor = max(XYZToRGB(inColor), 0.0);
    if (tonemap == 1)
        outColor = Reinhard(outColor);
    if (tonemap == 2)
        outColor = ACESFilm(outColor);
    if (tonemap == 3)
        outColor = DEUCESBioPhotometric(outColor);
    outColor = vec3(sRGBCompanding(outColor.x), sRGBCompanding(outColor.y), sRGBCompanding(outColor.z));

    return outColor;
}

void main() {
    int coords = int(gl_FragCoord.x) + resolution.x * int(gl_FragCoord.y);
    vec3 rendererColor = imageLoad(texelBuffer, coords).xyz;
    processorColor = vec4(Processing(rendererColor), 1.0);
}
