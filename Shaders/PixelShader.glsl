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
uniform vec3 lensData;
uniform int samplesPerFrame;
uniform int pathLength;
uniform float CIEXYZ2006[1323];
uniform int numObjects[];
uniform float objects[512];
uniform float materials[512];
uniform sampler2D screenTexture;

#define MAXDIST 1e5
#define PI 3.141592653589792623810034526344

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

struct lens {
	vec3 pos;
	vec3 rotation;
	float radius;
	float focalLength;
	bool isConverging;
	int materialID;
};

struct material {
	vec3 reflection;
	vec2 emission;
};

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

float InvSpectraPDF(in float start, in float end){
	// Inverse Of Uniform PDF
	return end - start;
}

// http://www.songho.ca/opengl/gl_anglestoaxes.html
mat3 RotationMatrix(in vec3 angle) {
	// Builds Rotation Matrix Depending On Given Angle
	angle *= 0.0174532925199;
	vec3 sinxyz = sin(angle);
	vec3 cosxyz = cos(angle);
	mat3 mX = mat3(1.0, 0.0, 0.0, 0.0, cosxyz.x, -sinxyz.x, 0.0, sinxyz.x, cosxyz.x);
	mat3 mY = mat3(cosxyz.y, 0.0, sinxyz.y, 0.0, 1.0, 0.0, -sinxyz.y, 0.0, cosxyz.y);
	mat3 mZ = mat3(cosxyz.z, -sinxyz.z, 0.0, sinxyz.z, cosxyz.z, 0.0, 0.0, 0.0, 1.0);
	return mX * mY * mZ;
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

// TODO: Return Booleans And Return Inside IFs To Get Results Little Faster
float SphereIntersection(in Ray ray, in vec3 pos, in float radius, out vec3 normal) {
	// Ray-Intersection Of Sphere
	// Built By Solving The Equation: x^2 + y^2 + z^2 = r^2
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
	normal = normalize(fma(ray.dir, vec3(t), localorigin) * isOutside);
	return t;
}

float PlaneIntersection(in Ray ray, in vec3 pos, out vec3 normal) {
	// Ray-Intersection Of Plane
	// Built By Solving The Equation: z = 0
	vec3 localorigin = ray.origin - pos;
	float t = -localorigin.y / ray.dir.y;
	t = (t < 1e-4) ? 1e6 : t;
	normal = vec3(0.0, 1.0, 0.0);
	normal = faceforward(normal, ray.dir, normal);
	return t;
}

// Box Intersection By iq
// https://www.shadertoy.com/view/ld23DV
float BoxIntersection(in Ray ray, in vec3 pos, in vec3 rotation, in vec3 size, out vec3 normal) {
	// Ray-Intersection Of Box
	mat3 matrix = RotationMatrix(rotation);
	vec3 localorigin = (ray.origin - pos) * matrix;
	ray.dir = ray.dir * matrix;
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
	normal = matrix * normalize(normal);
	return t;
}

float SphereSliceIntersection(in Ray ray, in vec3 pos, in vec3 rotation, in float radius, in float sliceSize, in bool is1stSlice, out vec3 normal, out int isOutside) {
	// Ray-Intersection Of Sliced Sphere
	// Slicing Based On Parameters Slice Size And Slice Side
	// Slicing Is Done Using 1D Interval Checks
	// Built By Solving The Same Equation Of Sphere
	mat3 matrix = RotationMatrix(rotation);
	Ray localRay;
	float sliceOffset = radius - sliceSize;
	localRay.origin = ray.origin - pos;
	localRay.origin.x += is1stSlice ? -sliceSize : sliceSize;
	localRay.origin = localRay.origin * matrix;
	localRay.origin.x += is1stSlice ? -sliceOffset : sliceOffset;
	localRay.dir = ray.dir * matrix;
	float a = dot(localRay.dir, localRay.dir);
	float b = 2.0 * dot(localRay.dir, localRay.origin);
	float c = dot(localRay.origin, localRay.origin) - (radius * radius);
	float discriminant = b * b - 4.0 * a * c;
	float t = 1e6;
	if (discriminant >= 0.0) {
		float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
		float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
		if (is1stSlice) {
			t1 = (fma(localRay.dir.x, t1, localRay.origin.x) > -sliceOffset) ? 1e6 : t1;
			t2 = (fma(localRay.dir.x, t2, localRay.origin.x) > -sliceOffset) ? 1e6 : t2;
		} else {
			t1 = (fma(localRay.dir.x, t1, localRay.origin.x) < sliceOffset) ? 1e6 : t1;
			t2 = (fma(localRay.dir.x, t2, localRay.origin.x) < sliceOffset) ? 1e6 : t2;
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
	normal = matrix * normalize(fma(localRay.dir, vec3(t), localRay.origin) * isOutside);
	return t;
}

void LensIntersection(in Ray ray, inout float hitdist, inout vec3 normal, inout int isOutside, inout material mat, in lens object) {
	// Ray-Intersection Of Lens
	// Done By Joining Two Slices Of Sphere
	// Thickness Of Lens Is Calculated By Using The Equation: thickness = 2(2f - sqrt(4f^2 - R^2))
	float lensThickness = 4.0 * object.focalLength - 2.0 * sqrt(4.0 * object.focalLength * object.focalLength - object.radius * object.radius);
	bool lensSlicePart[2] = {true, false};
	float lensSlicePos[2] = {0.0, 0.0};
	if (object.isConverging) {
		lensSlicePos[0] = lensThickness / 2.0;
		lensSlicePos[1] = -lensThickness / 2.0;
	}
	for (int i = 0; i < 2; i++) {
		vec3 norm = vec3(0.0);
		int isOut = 1;
		float t = SphereSliceIntersection(ray, object.pos - vec3(lensSlicePos[i], 0.0, 0.0), object.rotation, 2.0 * object.focalLength, lensThickness / 2.0, lensSlicePart[i], norm, isOut);
		if (t < hitdist) {
			hitdist = t;
			normal = norm;
			isOutside = object.isConverging ? isOut : (1 - isOut);
			mat = GetMaterial(object.materialID);
		}
	}
}

float Intersection(in Ray ray, out vec3 normal, out material mat) {
	// Finds The Ray-Intersection Of Every Object In The Scene
	float hitdist = 1e6;
	// Loop Over All The Spheres In The Scene
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
	// Loop Over All The Planes In The Scene
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
	// Loop Over All The Cubes In The Scene
	for (int i = 0; i < 1; i++) {
		int offset = 0;
		vec3 norm = vec3(0.0);
		float intersect = BoxIntersection(ray, vec3(3.0, 0.75, 1.0), vec3(0.0, 58.31, 0.0), vec3(1.5, 1.5, 1.5), norm);
		if (intersect < hitdist) {
			hitdist = intersect;
			normal = norm;
			mat = GetMaterial(0);
		}
	}
	
	lens object;
	object.pos = vec3(5.0, 1.3, -4.0);
	object.rotation = vec3(0.0, 0.0, 0.0);
	object.radius = 1.3;
	object.focalLength = 1.0;
	object.isConverging = true;
	object.materialID = 0;
	int isOutside = 1;
	LensIntersection(ray, hitdist, normal, isOutside, mat, object);

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

uint GenerateSeed(in uvec2 xy, in uint k) {
	// Actually This Is Not The Correct Way To Generate Seed
	// This Is The Correct Implementation Which Has No Overlapping:
	/* uint seed = (resolution.x * resolution.y) * (frame - 1) + k;
	   seed += xy.x + resolution.x * xy.y;*/
	// But Because This Seed Crosses 32-Bit Limit Quickly, And Implementing In 64-Bit Makes Path Tracer Much Slower,
	// That's Why I Implemented This Trick. Even If Pixels Seed Overlap With Other Pixels Somewhere, It Won't Affect The Result
	uint seed = (frame - 1) + k;
	PCG32(seed);
	seed = uint(mod(double(seed) + double(xy.x + resolution.x * xy.y), 0xFFFFFFFFu));
	return seed;
}

vec3 UniformRandomPointsUnitSphere(inout uint seed){
	// Generates Uniformly Distributed Random Points On Unit Sphere
	// XYZ Coordinates Is Calculated Based On Longitude And Latitude
	// Longitude Is Generated Uniformly And Sin Of Latitude Is Generated Uniformly
	// Reason: If We Generate Latitude Uniformly, The Top And Bottom Of The Sphere Will Have More Points Than Other Regions
	float phi = 2.0 * PI * RandomFloat(seed);
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

float RefractiveIndexBK7Glass(in float l) {
	// Sellmeier Equation For Refractive Index Of BK7 Glass
	l *= 1e-3;
	float l2 = l * l;
	float n2 = 1.0;
	n2 += (1.03961212 * l2) / (l2 - 6.00069867e-3);
	n2 += (0.231792344 * l2) / (l2 - 2.00179144e-2);
	n2 += (1.01046945 * l2) / (l2 - 1.03560653e2);
	return sqrt(n2);
}

float TraceRay(in float l, inout float rayradiance, inout Ray ray, inout uint seed, out bool isTerminate) {
	// Traces A Ray Along The Given Origin And Direction Then Calculates Light Interactions
	float radiance = 0.0;
	vec3 normal = vec3(0.0);
	material mat;
	float hitdist = Intersection(ray, normal, mat);
	if (hitdist < MAXDIST) {
		if (mat.emission.y > 0.0) {
			radiance = fma(Emit(l, mat), rayradiance, radiance);
			// Terminate The Path If The Ray Hits The Light Source
			isTerminate = true;
			return radiance;
		}
		rayradiance *= SpectralPowerDistribution(l, mat.reflection.x, mat.reflection.y, int(mat.reflection.z));
		// Russian Roulette
		// Probability Of The Ray Can Be Anything From 0 To 1
		float rayProbability = clamp(rayradiance, 0.0, 0.99);
		if (RandomFloat(seed) > rayProbability) {
			// Randomly Terminate Ray Based On Probability
			isTerminate = true;
			return radiance;
		}
		// Add Energy Which Was Lost By Terminating Rays
		rayradiance *= 1.0 / rayProbability;
		// Calculate The Next Ray's Origin And Direction
		ray.origin = fma(ray.dir, vec3(hitdist), ray.origin);
		ray.dir = RandomCosineDirectionHemisphere(seed, normal);
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

void TracePathLens(in float l, inout Ray ray, in vec3 forwardDir, out bool isTerminate) {
	// Trace The Path Through The Convex Lens
	// Therefore, We Get Physically Accurate Lens Distortion And Chromatic Aberration Effects
	lens object;
	object.radius = lensData.x;
	object.focalLength = lensData.y;
	object.isConverging = true;
	object.pos = ray.origin + forwardDir * lensData.z;
	object.rotation = vec3(0.0, 90.0 - cameraAngle.y, cameraAngle.x);
	object.materialID = 0;
	for (int i = 0; i < 2; i++) {
		float hitdist = 1e6;
		vec3 normal = vec3(0.0);
		int isOutside = 1;
		material mat;
		LensIntersection(ray, hitdist, normal, isOutside, mat, object);
		if (hitdist < MAXDIST) {
			// Calculate The Refractive Index
			float index = RefractiveIndexBK7Glass(l);
			index = (isOutside == 1) ? (1.0 / index) : index;
			// Calculate The Next Ray's Origin And Direction
			ray.origin = fma(ray.dir, vec3(hitdist), ray.origin);
			ray.dir = refract(ray.dir, normal, index);
		} else {
			isTerminate = true;
			break;
		}
	}
}

vec3 Scene(in uvec2 xy, in vec2 uv, in uint k) {
	uint seed = GenerateSeed(xy, k);
	// SSAA
	uv += vec2(2.0 * RandomFloat(seed) - 0.5, 2.0 * RandomFloat(seed) - 0.5) / resolution;

	Ray ray;
	ray.origin = cameraPos;
	mat3 matrix = RotationMatrix(vec3(cameraAngle, 0.0));
	ray.dir = normalize(vec3(uv, 1.0) * matrix);
	vec3 forwardDir = vec3(matrix[0][2], matrix[1][2], matrix[2][2]);

	vec3 color = vec3(0.0);

	float l = SampleSpectra(390.0, 720.0, RandomFloat(seed));
	bool isTerminate = false;
	TracePathLens(l, ray, forwardDir, isTerminate);
	if (!isTerminate) {
		color += (TracePath(l, ray, seed) * WaveToXYZ(l)) * InvSpectraPDF(390.0, 720.0);
	}

	return color;
}

void Accumulate(inout vec3 color) {
	// Temporal Accumulation Based On Given Parameters When Scene Is Dynamic And Accumulation When Scene Is Static
	// Simulation Of Persistance Using Temporal Accumulation
	// The Idea Is To Multiply Color Value By 1/256 In A Number Of Frames
	// Find The Constant Based On Given Parameters
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
	uvec2 xy = uvec2(gl_FragCoord.xy);
	vec2 uv = ((2.0 * vec2(xy) - resolution) / resolution.y) * tan(FOV * 0.00872664625997);

	vec3 color = vec3(0.0);
	for (uint i = 0; i < samplesPerFrame; i++) {
		color += Scene(xy, uv, i);
	}
	color *= exposure;
	Accumulate(color);

	Fragcolor = vec4(color, 1.0);
}