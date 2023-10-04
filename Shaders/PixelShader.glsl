#version 460 core
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout(location = 0) out vec4 Fragcolor;

uniform ivec2 resolution;
uniform int frame;
uniform int frame0;
uniform int prevFrame0;
uniform vec2 cameraAngle;
uniform vec3 cameraPos;
uniform float FOV;
uniform float apertureSize;
uniform int pathsPerFP;
uniform int pathLength;
uniform float CIEXYZ1964[1413];
uniform int numObjects[];
uniform float objects[512];
uniform float materials[512];
uniform sampler2D screenTexture;

struct sphere {
	vec3 pos;
	float radius;
	int materialID;
};

struct plane {
	vec3 pos;
	int materialID;
};

struct material {
	vec3 reflection;
	vec4 emission;
};

const float maxdist = 1e5;
const float pi = 3.141592653589792623810034526344;

vec3 WaveToXYZ(float wave) {
	// Conversion From Wavelength To XYZ Using CIE1964 XYZ Table
	vec3 XYZ = vec3(0.0, 0.0, 0.0);
	if ((wave >= 360) && (wave <= 830)){
		// Finding The Appropriate Index Of The Table For Given Wavelength
		int index = int((floor(wave) - 360.0));
		vec3 t1 = vec3(CIEXYZ1964[3*index], CIEXYZ1964[3*index+1], CIEXYZ1964[3*index+2]);
		vec3 t2 = vec3(CIEXYZ1964[3*index+3], CIEXYZ1964[3*index+4], CIEXYZ1964[3*index+5]);
		// Interpolation To Make The Colors Smooth
		XYZ = mix(t1, t2, wave - floor(wave));
	}
	return XYZ;
}

float SampleSpectral(float start, float end, float rand){
	// Uniform Inverted CDF For Sampling
	return (end - start) * rand + start;
}

float SpectralPDF(float start, float end){
	// Uniform PDF
	return 1.0 / (end - start);
}

mat3 RotateCamera(vec2 theta) {
	// http://www.songho.ca/opengl/gl_anglestoaxes.html
	mat3 mX = mat3(1.0, 0.0, 0.0, 0.0, cos(theta.x), -sin(theta.x), 0.0, sin(theta.x), cos(theta.x));
	mat3 mY = mat3(cos(theta.y), 0.0, sin(theta.y), 0.0, 1.0, 0.0, -sin(theta.y), 0.0, cos(theta.y));
	return mX * mY;
}

material GetMaterial(int index) {
	// Extract Material Data From The Materials List
	material material;
	material.reflection.x = materials[7*index];
	material.reflection.y = materials[7*index+1];
	material.reflection.z = materials[7*index+2];
	material.emission.x = materials[7*index+3];
	material.emission.y = materials[7*index+4];
	material.emission.z = materials[7*index+5];
	material.emission.w = materials[7*index+6];
	return material;
}

float SphereIntersection(vec3 origin, vec3 dir, vec3 pos, float radius, out vec3 normal) {
	vec3 localorigin = origin - pos;
	float a = dot(dir, dir);
	float b = 2.0 * dot(dir, localorigin);
	float c = dot(localorigin, localorigin) - (radius * radius);
	float discriminant = b * b - 4.0 * a * c;
	float t = 1e6;
	if (discriminant >= 0.0) {
		t = (-b - sqrt(discriminant)) / (2.0 * a);
		if (t < 0.0) {
			t = (-b + sqrt(discriminant)) / (2.0 * a);
		}
	}
	if (t < 1e-4) {
		t = 1e6;
	}
	normal = normalize((localorigin + dir*t) * radius);
	return t;
}

float PlaneIntersection(vec3 origin, vec3 dir, vec3 pos, out vec3 normal) {
	vec3 localorigin = origin - pos;
	float t = -localorigin.y / dir.y;
	if (t < 1e-4) {
		t = 1e6;
	}
	normal = vec3(0.0, 1.0, 0.0);
	normal = faceforward(normal, dir, normal);
	return t;
}

// Box Intersection By iq
// https://www.shadertoy.com/view/ld23DV
float BoxIntersection(vec3 origin, vec3 dir, vec3 size, vec3 pos, out vec3 normal) {
	vec3 localorigin = origin - pos;
	vec3 m = 1.0 / dir;
	vec3 n = m * localorigin;
	vec3 k = abs(m) * size / 2.0;
	vec3 k1 = -n - k;
	vec3 k2 = -n + k;
	float tN = max(k1.x, max(k1.y, k1.z));
	float tF = min(k2.x, min(k2.y, k2.z));
	float t1 = min(tN, tF);
	float t2 = max(tN, tF);
	float t = 1e6;
	if (t1 < 0) {
		t = t2;
	} else {
		t = t1;
	}
	if (tN > tF) {
		t = 1e6;
	}
	if (t < 1e-4) {
		t = 1e6;
	}
	if (tN > 0) {
		normal = step(vec3(tN), k1);
	} else {
		normal = step(k2, vec3(tF));
	}
	normal *= -sign(dir);
	normal = normalize(normal);
	return t;
}

float Intersection(vec3 origin, vec3 dir, out vec3 normal, out material mat) {
	float hitdist = 1e6;
	
	for (int i = 0; i < numObjects[0]; i++) {
		int offset = 0;
		sphere object;
		object.pos = vec3(objects[5*i+offset], objects[5*i+1+offset], objects[5*i+2+offset]);
		object.radius = objects[5*i+3+offset];
		object.materialID = int(objects[5*i+4+offset])-1;
		vec3 norm = vec3(0.0);
		float intersect = SphereIntersection(origin, dir, object.pos, object.radius, norm);
		if (intersect < hitdist) {
			hitdist = intersect;
			normal = norm;
			mat = GetMaterial(object.materialID);
		}
	}

	for (int i = 0; i < numObjects[1]; i++) {
		int offset = 5*numObjects[0];
		plane object;
		object.pos = vec3(objects[4*i+offset], objects[4*i+1+offset], objects[4*i+2+offset]);
		object.materialID = int(objects[4*i+3+offset])-1;
		vec3 norm = vec3(0.0);
		float intersect = PlaneIntersection(origin, dir, object.pos, norm);
		if (intersect < hitdist) {
			hitdist = intersect;
			normal = norm;
			mat = GetMaterial(object.materialID);
		}
	}
	/*
	for (int i = 0; i < 1; i++) {
		int offset = 0;
		vec3 norm = vec3(0.0);
		float intersect = BoxIntersection(origin, dir, vec3(2.0, 2.0, 2.0), vec3(0.0, 1.0, 0.0), norm);
		if (intersect < hitdist) {
			hitdist = intersect;
			normal = norm;
			mat.emission.x = 0.0;
			mat.emission.y = 0.0;
			mat.emission.z = 0.0;
			mat.emission.w = 0.0;
		}
	}
	*/
	return hitdist;
}

// https://www.pcg-random.org/
void PCG32(inout uint seed) {
	uint state = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	seed = (word >> 22u) ^ word;
}

float RandomFloat(inout uint seed) {
	PCG32(seed);
	return float(seed) / 0xFFFFFFFFu;
}

uint GenerateSeed(uint x, uint y, uint k) {
	// Actually This Is Not The Correct Way To Generate Seed.
	// This Is The Correct Implementation Which Has No Overlapping:
	/* uint seed = (resolution.x * resolution.y) * (pathsPerFP * (frame - 1)) + k;
	   seed += x + resolution.x * y;*/
	// But Because This Seed Crosses 32-Bit Limit Quickly, And Implementing In 64-Bit Makes Path Tracer Much Slower,
	// That's Why I Implemented This Trick. Even If Pixels Seed Overlap With Other Pixels Somewhere, It Won't Affect The Result.
	uint seed = (pathsPerFP * (frame - 1)) + k;
	PCG32(seed);
	seed = uint(mod(uint64_t(seed) + uint64_t(x + resolution.x * y), 0xFFFFFFFFul));
	return seed;
}

float RandomNormal(inout uint seed) {
	return cos(2.0 * pi * RandomFloat(seed)) * sqrt(-2.0 * log(RandomFloat(seed)) / log(10.0));
}

vec3 UniformRandomPointsUnitSphere(inout uint seed){
	// Generate Random Points On Unit Sphere
	float x = RandomNormal(seed);
	float y = RandomNormal(seed);
	float z = RandomNormal(seed);
	return normalize(vec3(x, y, z));
}

vec3 RandomCosineDirectionHemisphere(inout uint seed, vec3 normal){
	// Generate Cosine Distributed Random Vectors Within Normals Hemisphere
	vec3 sumvector = normal + UniformRandomPointsUnitSphere(seed);
	return normalize(sumvector);
}

float SpectralPowerDistribution(float l, float l_peak, float d, int invert) {
	// Spectral Power Distribution Function Calculated On The Basis Of Peak Wavelength And Standard Deviation
	// Using Gaussian Function To Predict Spectral Radiance
	// In Reality, Spectral Radiance Function Has Different Shapes For Different Objects Also Looks Much Different Than This
	float radiance = exp(-pow(((l - l_peak) / (2 * d * d)), 2));
	radiance = mix(radiance, 1.0 - radiance, invert);
	return radiance;
}

float RefractiveIndexWavelength(float l, float n, float l_n, float s){
	// My Own Function For Refractive Index
	// Function Is Based On Observation How Graph Of Mathematrical Functions Look Like
	// Made To Produce Change In Refractive Index Based On Wavelength
	return s * ((l_n / l) - 1.0) + n;
}

float Emit(float l, material mat) {
	// Calculates Light Emittance Based On Given Material
	float lightEmission = SpectralPowerDistribution(l, mat.emission.x, mat.emission.y, int(mat.emission.z)) * max(mat.emission.w, 0.0);
	return lightEmission;
}

float TraceRay(in float l, inout float rayradiance, inout vec3 origin, inout vec3 dir, inout uint seed, out bool isTerminate) {
	// Traces A Ray Along The Given Origin And Direction Then Calculates Light Interactions
	float radiance = 0.0;
	vec3 normal = vec3(0.0);
	material mat;
	float hitdist = Intersection(origin, dir, normal, mat);
	if (hitdist < maxdist) {
		origin = origin + dir * hitdist;
		dir = RandomCosineDirectionHemisphere(seed, normal);
		if (mat.emission.w > 0.0) {
			radiance += Emit(l, mat) * rayradiance;
			isTerminate = true;
		}
		rayradiance *= SpectralPowerDistribution(l, mat.reflection.x, mat.reflection.y, int(mat.reflection.z));
	}
	else {
		isTerminate = true;
	}

	return radiance;
}

float TracePath(in float l, in vec3 origin, in vec3 dir, inout uint seed) {
	// Traces A Path Starting From The Given Origin And Direction
	// And Calculates Light Radiance
	float radiance = 0.0;
	float rayradiance = 1.0;
	bool isTerminate = false;
	for (int i = 0; i < pathLength; i++) {
		radiance += TraceRay(l, rayradiance, origin, dir, seed, isTerminate);
		if (isTerminate){
			break;
		}
	}
	return radiance;
}

vec3 Scene(uint x, uint y, uint k) {
	uint seed = GenerateSeed(x, y, k);
	float tan_fov = tan(radians(FOV / 2.0));
	vec2 uv = ((vec2(x, y) * 2.0 - resolution) / resolution.y) * tan_fov;
	// SSAA
	uv += vec2(2.0 * RandomFloat(seed) - 0.5, 2.0 * RandomFloat(seed) - 0.5) / resolution;
	// Fix Camera Angle For The Rotation Function
	vec2 theta = vec2(-cameraAngle.y, cameraAngle.x);

	vec3 origin = cameraPos;
	mat3 m1 = RotateCamera(theta); // Can Be Optimized By Computing This Outside The Fragment Shader
	vec3 m2 = vec3(uv, 1.0);
	vec3 dir = normalize(vec3(m2 * m1));
	vec3 forward = normalize(vec3(vec3(0.0, 0.0, 1.0) * m1));

	vec3 color = vec3(0.0);

	float l = SampleSpectral(380.0, 720.0, RandomFloat(seed));
	// Chromatic Aberration
	dir = refract(dir, -forward, 1.0 / RefractiveIndexWavelength(l, 1.010, 550.0, 0.025));
	color += (TracePath(l, origin, dir, seed) * WaveToXYZ(l)) / SpectralPDF(380.0, 720.0);
	color *= apertureSize * apertureSize;

	return color;
}

void main() {
	vec4 color = vec4(0.0);
	for (uint i = 0; i < pathsPerFP; i++) {
		color.xyz += Scene(uint(gl_FragCoord.x), uint(gl_FragCoord.y), i);
	}
	color.xyz /= pathsPerFP;
	if (frame0 < 2) {
		color = 0.5 * texture(screenTexture, gl_FragCoord.xy / resolution) / prevFrame0 + 0.5 * color;
	} else {
		color = texture(screenTexture, gl_FragCoord.xy / resolution) + color;
	}
	Fragcolor = color;
}