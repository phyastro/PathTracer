#version 460 core

layout(location = 0) out vec4 Fragcolor;

uniform ivec2 resolution;
uniform int frame;
uniform int samples;
uniform int prevSamples;
uniform float FPS;
uniform vec2 cameraAngle;
uniform vec3 cameraPos;
uniform float FOV;
uniform float persistance;
uniform float exposure;
uniform int samplesPerFrame;
uniform int pathLength;
uniform float CIEXYZ2006[1323];
uniform int numObjects[];
uniform float objects[512];
uniform float materials[512];
uniform sampler2D screenTexture;

struct Ray {
	vec3 origin;
	vec3 dir;
};

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
	vec2 emission;
};

const float maxdist = 1e5;
const float pi = 3.141592653589792623810034526344;

vec3 WaveToXYZ(in float wave) {
	// Conversion From Wavelength To XYZ Using CIE1964 XYZ Table
	vec3 XYZ = vec3(0.0, 0.0, 0.0);
	if ((wave >= 390) && (wave <= 830)){
		// Finding The Appropriate Index Of The Table For Given Wavelength
		int index = int((floor(wave) - 390.0));
		vec3 t1 = vec3(CIEXYZ2006[3*index], CIEXYZ2006[3*index+1], CIEXYZ2006[3*index+2]);
		vec3 t2 = vec3(CIEXYZ2006[3*index+3], CIEXYZ2006[3*index+4], CIEXYZ2006[3*index+5]);
		// Interpolation To Make The Colors Smooth
		XYZ = mix(t1, t2, wave - floor(wave));
	}
	return XYZ;
}

float SampleSpectra(in float start, in float end, in float rand){
	// Uniform Inverted CDF For Sampling
	return (end - start) * rand + start;
}

float SpectraPDF(in float start, in float end){
	// Uniform PDF
	return 1.0 / (end - start);
}

// http://www.songho.ca/opengl/gl_anglestoaxes.html
mat3 RotateCamera(in vec2 theta) {
	vec2 sinxy = sin(theta);
	vec2 cosxy = cos(theta);
	mat3 mX = mat3(1.0, 0.0, 0.0, 0.0, cosxy.x, -sinxy.x, 0.0, sinxy.x, cosxy.x);
	mat3 mY = mat3(cosxy.y, 0.0, sinxy.y, 0.0, 1.0, 0.0, -sinxy.y, 0.0, cosxy.y);
	return mX * mY;
}

// http://www.songho.ca/opengl/gl_anglestoaxes.html
vec3 RotateVector(in vec3 vector, in vec3 angle) {
	vec3 sinxyz = sin(angle);
	vec3 cosxyz = cos(angle);
	mat3 mX = mat3(1.0, 0.0, 0.0, 0.0, cosxyz.x, -sinxyz.x, 0.0, sinxyz.x, cosxyz.x);
	mat3 mY = mat3(cosxyz.y, 0.0, sinxyz.y, 0.0, 1.0, 0.0, -sinxyz.y, 0.0, cosxyz.y);
	mat3 mZ = mat3(cosxyz.z, -sinxyz.z, 0.0, sinxyz.z, cosxyz.z, 0.0, 0.0, 0.0, 1.0);
	mat3 m = mX * mY * mZ;
	return vector * m;
}

material GetMaterial(in int index) {
	// Extract Material Data From The Materials List
	material material;
	material.reflection.x = materials[5*index];
	material.reflection.y = materials[5*index+1];
	material.reflection.z = materials[5*index+2];
	material.emission.x = materials[5*index+3];
	material.emission.y = materials[5*index+4];
	return material;
}

float SphereIntersection(in Ray ray, in vec3 pos, in float radius, out vec3 normal) {
	vec3 localorigin = ray.origin - pos;
	float a = dot(ray.dir, ray.dir);
	float b = 2.0 * dot(ray.dir, localorigin);
	float c = dot(localorigin, localorigin) - (radius * radius);
	float discriminant = b * b - 4.0 * a * c;
	float t = 1e6;
	int isOutside = 1;
	if (discriminant >= 0.0) {
		float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
		float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
		t = (t1 > 0.0) ? t1 : t2;
		isOutside = (t1 > 0.0) ? 1 : -1;
	}
	if (t < 1e-4) {
		t = 1e6;
		isOutside = 1;
	}
	normal = normalize(fma(ray.dir, vec3(t), localorigin) * radius * isOutside);
	return t;
}

float PlaneIntersection(in Ray ray, in vec3 pos, out vec3 normal) {
	vec3 localorigin = ray.origin - pos;
	float t = -localorigin.y / ray.dir.y;
	t = (t < 1e-4) ? 1e6 : t;
	normal = vec3(0.0, 1.0, 0.0);
	normal = faceforward(normal, ray.dir, normal);
	return t;
}

// Box Intersection By iq
// https://www.shadertoy.com/view/ld23DV
float BoxIntersection(in Ray ray, in vec3 pos, in vec3 size, out vec3 normal) {
	vec3 localorigin = RotateVector(ray.origin - pos, vec3(0.0, 58.31 * pi / 180.0, 0.0));
	ray.dir = RotateVector(ray.dir, vec3(0.0, 58.31 * pi / 180.0, 0.0));
	vec3 m = 1.0 / ray.dir;
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
	normal *= -sign(ray.dir);
	normal = RotateVector(normalize(normal), -vec3(0.0, 58.31 * pi / 180.0, 0.0));
	return t;
}

float SphereSliceIntersection(in Ray ray, in vec3 pos, in float radius, in float sliceSize, in bool is1stSlice, out vec3 normal) {
	float carveOffset = radius - sliceSize;
	vec3 localorigin = ray.origin - pos;
	if (is1stSlice) {
		localorigin.x -= carveOffset + sliceSize;
	} else {
		localorigin.x += carveOffset + sliceSize;
	}
	float a = dot(ray.dir, ray.dir);
	float b = 2.0 * dot(ray.dir, localorigin);
	float c = dot(localorigin, localorigin) - (radius * radius);
	float discriminant = b * b - 4.0 * a * c;
	float t = 1e6;
	int isOutside = 1;
	if (discriminant >= 0.0) {
		float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
		float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
		if (is1stSlice) {
			t1 = (fma(ray.dir.x, t1, localorigin.x) > -carveOffset) ? 1e6 : t1;
			t2 = (fma(ray.dir.x, t2, localorigin.x) > -carveOffset) ? 1e6 : t2;
		} else {
			t1 = (fma(ray.dir.x, t1, localorigin.x) < carveOffset) ? 1e6 : t1;
			t2 = (fma(ray.dir.x, t2, localorigin.x) < carveOffset) ? 1e6 : t2;
		}
		t = (t1 > 0.0) ? t1 : t;
		if (t2 < t) {
			t = t2;
			isOutside = -1;
		}
	}
	if (t < 1e-4) {
		t = 1e6;
		isOutside = 1;
	}
	normal = normalize(fma(ray.dir, vec3(t), localorigin) * radius * isOutside);
	return t;
}

float lensRadius = 1.0;
float lensFocalLength = 1.0;
float lensThickness = 4.0 * lensFocalLength - 2.0 * sqrt(4.0 * lensFocalLength * lensFocalLength - lensRadius * lensRadius);

void LensIntersection(inout float hitdist, in Ray ray, in vec3 pos, inout vec3 normal, inout material mat) {
	bool lensSlicePart[2] = {true, false};
	float lensSlicePos[2] = {lensThickness / 2.0, -lensThickness / 2.0};
	for (int i = 0; i < 2; i++) {
		vec3 norm = vec3(0.0);
		float t = SphereSliceIntersection(ray, pos - vec3(lensSlicePos[i], 0.0, 0.0), 2.0 * lensFocalLength, lensThickness / 2.0, lensSlicePart[i], norm);
		if (t < hitdist) {
			hitdist = t;
			normal = norm;
			mat = GetMaterial(0);
		}
	}
}

float Intersection(in Ray ray, out vec3 normal, out material mat) {
	float hitdist = 1e6;
	
	for (int i = 0; i < numObjects[0]; i++) {
		int offset = 0;
		sphere object;
		object.pos = vec3(objects[5*i+offset], objects[5*i+1+offset], objects[5*i+2+offset]);
		object.radius = objects[5*i+3+offset];
		object.materialID = int(objects[5*i+4+offset])-1;
		vec3 norm = vec3(0.0);
		float intersect = SphereIntersection(ray, object.pos, object.radius, norm);
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
		float intersect = PlaneIntersection(ray, object.pos, norm);
		if (intersect < hitdist) {
			hitdist = intersect;
			normal = norm;
			mat = GetMaterial(object.materialID);
		}
	}
	
	for (int i = 0; i < 1; i++) {
		int offset = 0;
		vec3 norm = vec3(0.0);
		float intersect = BoxIntersection(ray, vec3(3.0, 0.75, 1.0), vec3(1.5, 1.5, 1.5), norm);
		if (intersect < hitdist) {
			hitdist = intersect;
			normal = norm;
			mat = GetMaterial(0);
		}
	}
	
	LensIntersection(hitdist, ray, vec3(5.0, 1.0, -4.0), normal, mat);

	return hitdist;
}

// https://www.pcg-random.org/
void PCG32(inout uint seed) {
	uint state = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	seed = (word >> 22u) ^ word;
}

/*
void LCG(inout uint seed) {
	// LCG PRNG Test
	// Produces Significant Artifacts Especially On The Light Sources And 3ms Slower(250FPS->100FPS)
	// Therefore, This Method Can Get Discarded
	seed = uint(mod(double(seed) * 983478477u, 0xFFFFFFFFu));
}
*/

float RandomFloat(inout uint seed) {
	PCG32(seed);
	return float(seed) / 0xFFFFFFFFu;
}

uint GenerateSeed(in uint x, in uint y, in uint k) {
	// Actually This Is Not The Correct Way To Generate Seed
	// This Is The Correct Implementation Which Has No Overlapping:
	/* uint seed = (resolution.x * resolution.y) * (frame - 1) + k;
	   seed += x + resolution.x * y;*/
	// But Because This Seed Crosses 32-Bit Limit Quickly, And Implementing In 64-Bit Makes Path Tracer Much Slower,
	// That's Why I Implemented This Trick. Even If Pixels Seed Overlap With Other Pixels Somewhere, It Won't Affect The Result
	uint seed = (frame - 1) + k;
	PCG32(seed);
	seed = uint(mod(double(seed) + double(x + resolution.x * y), 0xFFFFFFFFu));
	return seed;
}

vec3 UniformRandomPointsUnitSphere(inout uint seed){
	// Generates Uniformly Distributed Random Points On Unit Sphere
	// XYZ Coordinates Is Calculated Based On Longitude And Latitude
	// Longitude Is Generated Uniformly And Sin Of Latitude Is Generated Uniformly
	// Reason: If We Generate Latitude Uniformly, The Top And Bottom Of The Sphere Will Have More Points Than Other Regions
	float phi = 2.0 * pi * RandomFloat(seed);
	float sintheta = 2.0 * RandomFloat(seed) - 1.0;
	float costheta = sqrt(1.0 - sintheta * sintheta);
	float x = cos(phi) * costheta;
	float y = sin(phi) * costheta;
	float z = sintheta;
	return vec3(x, y, z);
}

vec3 RandomCosineDirectionHemisphere(inout uint seed, in vec3 normal){
	// Generate Cosine Distributed Random Vectors Within Normals Hemisphere
	vec3 sumvector = normal + UniformRandomPointsUnitSphere(seed);
	return normalize(sumvector);
}

float SpectralPowerDistribution(in float l, in float l_peak, in float d, in int invert) {
	// Spectral Power Distribution Function Calculated On The Basis Of Peak Wavelength And Standard Deviation
	// Using Gaussian Function To Predict Spectral Radiance
	// In Reality, Spectral Radiance Function Has Different Shapes For Different Objects Also Looks Much Different Than This
	float x = (l - l_peak) / (2.0 * d * d);
	float radiance = exp(-x * x);
	radiance = mix(radiance, 1.0 - radiance, invert);
	return radiance;
}

float BlackBodyRadiation(in float l, in float T) {
	// Plank's Law
	return (1.1910429724e-16 * pow(l, -5.0)) / (exp(0.014387768775 / (l * T)) - 1.0);
}

float BlackBodyRadiationPeak(in float T) {
	// Derived By Substituting Wien's Displacement Law On Plank's Law
	return 4.0956746759e-6 * pow(T, 5.0);
}

float Emit(in float l, in material mat) {
	// Calculates Light Emittance Based On Given Material
	float temperature = max(mat.emission.x, 0.0);
	float lightEmission = (BlackBodyRadiation(l * 1e-9, temperature) / BlackBodyRadiationPeak(temperature)) * max(mat.emission.y, 0.0);
	return lightEmission;
}

float RefractiveIndexWavelength(in float l, in float n, in float l_n, in float s){
	// My Own Function For Refractive Index
	// Function Is Based On Observation How Graph Of Mathematrical Functions Look Like
	// Made To Produce Change In Refractive Index Based On Wavelength
	return fma(s, (l_n / l) - 1.0, n);
}

float TraceRay(in float l, inout float rayradiance, inout Ray ray, inout uint seed, out bool isTerminate) {
	// Traces A Ray Along The Given Origin And Direction Then Calculates Light Interactions
	float radiance = 0.0;
	vec3 normal = vec3(0.0);
	material mat;
	float hitdist = Intersection(ray, normal, mat);
	if (hitdist < maxdist) {
		ray.origin = fma(ray.dir, vec3(hitdist), ray.origin);
		ray.dir = RandomCosineDirectionHemisphere(seed, normal);
		if (mat.emission.y > 0.0) {
			radiance = fma(Emit(l, mat), rayradiance, radiance);
			isTerminate = true;
		}
		rayradiance *= SpectralPowerDistribution(l, mat.reflection.x, mat.reflection.y, int(mat.reflection.z));
	}
	else {
		isTerminate = true;
	}

	return radiance;
}

float TracePath(in float l, in Ray ray, inout uint seed) {
	// Traces A Path Starting From The Given Origin And Direction
	// And Calculates Light Radiance
	float radiance = 0.0;
	float rayradiance = 1.0;
	bool isTerminate = false;
	for (int i = 0; i < pathLength; i++) {
		radiance += TraceRay(l, rayradiance, ray, seed, isTerminate);
		if (isTerminate){
			break;
		}
	}
	return radiance;
}

vec3 Scene(in uint x, in uint y, in uint k) {
	uint seed = GenerateSeed(x, y, k);
	float tan_fov = tan(radians(FOV / 2.0));
	vec2 uv = ((2.0 * vec2(x, y) - resolution) / resolution.y) * tan_fov;
	// SSAA
	uv += vec2(2.0 * RandomFloat(seed) - 0.5, 2.0 * RandomFloat(seed) - 0.5) / resolution;
	// Fix Camera Angle For The Rotation Function
	vec2 theta = vec2(-cameraAngle.y, cameraAngle.x);

	Ray ray;
	ray.origin = cameraPos;
	mat3 m1 = RotateCamera(theta); // Can Be Optimized By Computing This Outside The Fragment Shader
	vec3 m2 = vec3(uv, 1.0);
	ray.dir = normalize(vec3(m2 * m1));
	vec3 forward = normalize(vec3(vec3(0.0, 0.0, 1.0) * m1));

	vec3 color = vec3(0.0);

	float l = SampleSpectra(390.0, 720.0, RandomFloat(seed));
	// Chromatic Aberration
	//float hitdist = 1e6;
	//vec3 normal = vec3(0.0);
	//material mat;
	//LensIntersection(hitdist, ray, forward * 2.0, normal, mat);
	ray.dir = refract(ray.dir, -forward, 1.0 / RefractiveIndexWavelength(l, 1.010, 550.0, 0.025));
	color += (TracePath(l, ray, seed) * WaveToXYZ(l)) / SpectraPDF(390.0, 720.0);
	color *= exposure;
	//vec3 normal = vec3(0.0);
	//material mat;
	//Intersection(origin, dir, normal, mat);
	//color += normal;

	return color;
}

void Accumulate(inout vec3 color) {
	// Temporal Accumulation Based On Given Parameters When Scene Is Dynamic And Accumulation When Scene Is Static
	// Simulation Of Persistance Using Temporal Accumulation
	// The Idea Is To Multiply Color Value By 1/256 In A Number Of Frames
	// To Do That, Multiply A Constant[0, 1] Each Frame
	// Find That Constant Based On Given Parameters
	// Then We Get, x^numFrames = 2^(-8) Where x Is Multiply Constant
	// Also We Know That, numFrames = FPS * time(The Amount Of Time Will Be Needed To Reach 1/256)
	// Therefore, x = 2^(-8 / (FPS*time))
	if (prevSamples > 0) {
		if ((samples == samplesPerFrame) && (frame > samplesPerFrame)) {
			float weight = pow(2.0, -8.0 / (FPS * persistance));
			color = ((1.0 - weight) * color) + (weight * texture(screenTexture, gl_FragCoord.xy / resolution).xyz * samplesPerFrame / prevSamples);
		} else {
			color = texture(screenTexture, gl_FragCoord.xy / resolution).xyz + color;
		}
	}
}

void main() {
	vec3 color = vec3(0.0);
	for (uint i = 0; i < samplesPerFrame; i++) {
		color += Scene(uint(gl_FragCoord.x), uint(gl_FragCoord.y), i);
	}
	Accumulate(color);
	Fragcolor = vec4(color, 1.0);
}