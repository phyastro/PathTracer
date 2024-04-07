#version 460 core

layout(location = 0) out vec4 Fragcolor;

uniform ivec2 resolution;
uniform int frame;
uniform int samples;
uniform int prevSamples;
uniform float FPS;
uniform vec2 cameraAngle;
uniform vec3 cameraPos;
uniform float persistance;
uniform int ISO;
uniform float cameraSize;
uniform float apertureSize;
uniform float apertureDist;
uniform vec4 lensData;
uniform int samplesPerFrame;
uniform int pathLength;
uniform float CIEXYZ2006[1323];
uniform int numObjects[];
uniform float objects[100];
uniform float materials[100];
uniform sampler2D screenTexture;

#define MAXDIST 1e5
#define PI 3.141592653589792623810034526344
#define ONEBYTHREE 0.3333333

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

struct box {
	vec3 pos;
	vec3 rotation;
	vec3 size;
	int materialID;
};

struct sphereSlice {
	vec3 pos;
	float radius;
	float sliceSize;
	bool is1stSlice;
	vec3 rotation;
	int materialID;
};

struct lens {
	vec3 pos;
	vec3 rotation;
	float radius;
	float focalLength;
	float thickness;
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
		XYZ = mix(t1, t2, wave - floor(wave));
	}
	return XYZ;
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

void SortMinMax(inout vec3 t1, inout vec3 t2) {
	vec3 temp_t1 = t1;
	vec3 temp_t2 = t2;
	t1 = min(temp_t1, temp_t2);
	t2 = max(temp_t1, temp_t2);
}

bool BoundingSphere(in Ray ray, in vec3 pos, in float radius2) {
	// Bounding Sphere To Check Whether Ray Is Going To Hit The Object
	// Checks Sign Of Discriminant To Check Sphere Visibility And Removes The Fake Sphere Visible In The Opposite Direction
	vec3 localorigin = ray.origin - pos;
	float b = dot(ray.dir, localorigin);
	float c = dot(localorigin, localorigin) - radius2;
	if ((b * b) < c) {
		return false;
	}
	if ((b >= 0.0) && (c >= 0.0)) {
		return false;
	}
	return true;
}

bool RayIntersectAABB(in vec3 origin, in vec3 invdir, in box object) {
	// Checks Whether The Ray Is Intersecting Given Box
	vec3 localorigin = origin - object.pos;
	vec3 tMin = fma(object.size, vec3(-0.5), -localorigin) * invdir;
	vec3 tMax = fma(object.size, vec3(0.5), -localorigin) * invdir;
	SortMinMax(tMin, tMax);
	float t1 = max(max(tMin.x, tMin.y), tMin.z);
	float t2 = min(min(tMax.x, tMax.y), tMax.z);
	if ((t1 > t2) || ((t1 < 0.0) && (t2 < 0.0))) {
		return false;
	}
	return true;
}

bool SphereIntersection(in Ray ray, in sphere object, inout float hitdist, inout vec3 normal, inout material mat) {
	// Ray-Intersection Of Sphere
	// Built By Solving The Equation: x^2 + y^2 + z^2 = r^2
	vec3 localorigin = ray.origin - object.pos;
	float b = 2.0 * dot(ray.dir, localorigin);
	float c = dot(localorigin, localorigin) - (object.radius * object.radius);
	float discriminant = b * b - 4.0 * c;
	float t = 1e6;
	int isOutside = 1;
	if (discriminant < 0.0) {
		return false;
	}
	float sqrtD = sqrt(discriminant);
	float t1 = (-b - sqrtD) * 0.5;
	float t2 = (-b + sqrtD) * 0.5;
	t = (t1 > 0.0) ? t1 : t2;
	isOutside = (t1 > 0.0) ? 1 : -1;
	if (t < 1e-4) {
		return false;
	}
	if (t < hitdist) {
		hitdist = t;
		normal = normalize(fma(ray.dir, vec3(t), localorigin) * isOutside);
		mat = GetMaterial(object.materialID);
		return true;
	}
	return false;
}

bool PlaneIntersection(in Ray ray, in plane object, inout float hitdist, inout vec3 normal, inout material mat) {
	// Ray-Intersection Of Plane
	// Built By Solving The Equation: z = 0
	vec3 localorigin = ray.origin - object.pos;
	float t = -localorigin.y / ray.dir.y;
	if (t < 1e-4) {
		return false;
	}
	if (t < hitdist) {
		hitdist = t;
		normal = faceforward(vec3(0.0, 1.0, 0.0), ray.dir, vec3(0.0, 1.0, 0.0));
		mat = GetMaterial(object.materialID);
		return true;
	}
	return false;
}

bool BoxIntersection(in Ray ray, in box object, inout float hitdist, inout vec3 normal, inout material mat) {
	// Ray-Intersection Of Box
	mat3 matrix = RotationMatrix(object.rotation);
	vec3 localorigin = (ray.origin - object.pos) * matrix;
	ray.dir *= matrix;
	vec3 invdir = 1.0 / ray.dir;
	vec3 tMin = fma(object.size, vec3(-0.5), -localorigin) * invdir;
	vec3 tMax = fma(object.size, vec3(0.5), -localorigin) * invdir;
	SortMinMax(tMin, tMax);
	float t1 = max(max(tMin.x, tMin.y), tMin.z);
	float t2 = min(min(tMax.x, tMax.y), tMax.z);
	float t = (t1 < 0.0) ? t2 : t1;
	// Ray Didn't Intersect If (tMin.x > tMax.y) || (tMin.x > tMax.z) || (tMin.y > tMax.x) || (tMin.y > tMax.z) || (tMin.z > tMax.x) || (tMin.z > tMax.y)
	// Or If t1 And t2 Were Negative
	if ((t1 > t2) || (t < 1e-4)) {
		return false;
	}
	if (t < hitdist) {
		hitdist = t;
		// The Signed Component Of p Which Has Highest Magnitude Is The Normal
		vec3 p = abs(localorigin + ray.dir * t);
		normal = matrix * (step(max(max(p.x, p.y), p.z), p) * -sign(ray.dir));
		mat = GetMaterial(object.materialID);
		return true;
	}
	return false;
}

bool SphereSliceIntersection(in Ray ray, in sphereSlice object, in float localSlicePos, in bool isSideInvert, inout float hitdist, inout vec3 normal, inout int isOutside, inout material mat) {
	// Ray-Intersection Of Sliced Sphere
	// Slicing Based On Parameters Slice Size And Slice Side
	// Slicing Is Done Using 1D Interval Checks
	// Built By Solving The Same Equation Of Sphere
	mat3 matrix = RotationMatrix(object.rotation);
	Ray localRay;
	float sliceOffset = object.radius - object.sliceSize;
	localRay.origin = ray.origin - object.pos;
	localRay.origin = localRay.origin * matrix;
	if (object.is1stSlice) {
		localRay.origin.x += localSlicePos - object.sliceSize - sliceOffset;
	} else {
		localRay.origin.x -= localSlicePos - object.sliceSize - sliceOffset;
	}
	localRay.dir = ray.dir * matrix;
	float b = 2.0 * dot(localRay.dir, localRay.origin);
	float c = dot(localRay.origin, localRay.origin) - (object.radius * object.radius);
	float discriminant = b * b - 4.0 * c;
	float t = 1e6;
	int isOut = 1;
	if (discriminant < 0.0) {
		return false;
	}
	float sqrtD = sqrt(discriminant);
	float t1 = (-b - sqrtD) * 0.5;
	float t2 = (-b + sqrtD) * 0.5;
	if (object.is1stSlice) {
		t1 = (fma(localRay.dir.x, t1, localRay.origin.x) > -sliceOffset) ? 1e6 : t1;
		t2 = (fma(localRay.dir.x, t2, localRay.origin.x) > -sliceOffset) ? 1e6 : t2;
	} else {
		t1 = (fma(localRay.dir.x, t1, localRay.origin.x) < sliceOffset) ? 1e6 : t1;
		t2 = (fma(localRay.dir.x, t2, localRay.origin.x) < sliceOffset) ? 1e6 : t2;
	}
	t = (t1 > 0.0) ? t1 : t;
	if (t2 < t) {
		t = t2;
		isOut = -1;
	}
	if (t < 1e-4) {
		return false;
	}
	if (t < hitdist) {
		hitdist = t;
		normal = matrix * normalize(fma(localRay.dir, vec3(t), localRay.origin) * isOut);
		isOutside = (!isSideInvert) ? isOut : -isOut;
		mat = GetMaterial(object.materialID);
		return true;
	}
	return false;
}

void LensIntersection(in Ray ray, in lens object, inout float hitdist, inout vec3 normal, inout int isOutside, inout material mat) {
	// Ray-Intersection Of Lens
	// Done By Joining Two Slices Of Sphere
	// Thickness Of Lens Is Calculated By Using The Equation: thickness = 2(2f - sqrt(4f^2 - R^2))
	float lensThicknessHalf = 2.0 * object.focalLength - sqrt(4.0 * object.focalLength * object.focalLength - object.radius * object.radius);
	bool lensSlicePart[2] = {true, false};
	float lensSlicePos = 0.5 * (object.isConverging ? object.thickness : -object.thickness); // Gap Between Slices Of Lens
	if (object.isConverging) {
		lensSlicePos += lensThicknessHalf;
	}
	for (int i = 0; i < 2; i++) {
		sphereSlice slice;
		slice.pos = object.pos;
		slice.radius = 2.0 * object.focalLength;
		slice.sliceSize = lensThicknessHalf;
		slice.is1stSlice = lensSlicePart[i];
		slice.rotation = object.rotation;
		slice.materialID = object.materialID;
		SphereSliceIntersection(ray, slice, lensSlicePos, !object.isConverging, hitdist, normal, isOutside, mat);
	}
}

float evalQuadratic(in float a, in float b, in float c, in float x) {
	return x * (x * a + b) + c;
}

vec3 evalQuadratic(in float a, in float b, in float c, in vec3 x) {
	return x * (x * a + b) + c;
}

float evalCubic(in float a, in float b, in float c, in float d, in float x) {
	return x * (x * (x * a + b) + c) + d;
}

vec2 evalCubic(in float a, in float b, in float c, in float d, in vec2 x) {
	return x * (x * (x * a + b) + c) + d;
}

vec3 evalCubic(in float a, in float b, in float c, in float d, in vec3 x) {
	return x * (x * (x * a + b) + c) + d;
}

vec2 evalQuartic(in float a, in float b, in float c, in float d, in float e, in vec2 x) {
	return x * (x * (x * (x * a + b) + c) + d) + e;
}

bvec3 solveCubic(in float b, in float c, in float d, inout vec3 roots) {
    // https://arxiv.org/abs/1903.10041
    // Solves Cubic Equation By Combining Two Different Methods To Find Real Roots
    float bdiv3 = b * ONEBYTHREE;
    float Q = c * ONEBYTHREE - bdiv3 * bdiv3;
    float R = 0.5 * bdiv3 * c - bdiv3 * bdiv3 * bdiv3 - 0.5 * d;
	float D = Q * Q * Q + R * R;
	if (D > 0.0) {
		// Fix Issue In Calculating Signs Using pow For Cube Root
		float u = R + sqrt(D);
		float v = R - sqrt(D);
		float S = sign(u) * pow(abs(u), ONEBYTHREE);
		float T = sign(v) * pow(abs(v), ONEBYTHREE);
		roots.x = S + T - bdiv3;
		// Apply Newton Raphson Method 2 Times To Increase The Accuracy
		for (int i = 0; i < 2; i++) {
			roots.x -= evalCubic(1.0, b, c, d, roots.x) / evalQuadratic(3.0, 2.0 * b, c, roots.x);
		}
		return bvec3(true, false, false);
	}
	float sqrtnegQ = sqrt(-Q);
	float thetadiv3 = acos(R / (sqrtnegQ * sqrtnegQ * sqrtnegQ)) * ONEBYTHREE;
	float TWOPIBYTHREE = fma(2.0, PI, ONEBYTHREE);
	roots = 2.0 * sqrtnegQ * vec3(cos(thetadiv3), cos(thetadiv3 + TWOPIBYTHREE), cos(fma(2.0, TWOPIBYTHREE, thetadiv3))) - bdiv3;
	// Apply Newton Raphson Method 2 Times To Increase The Accuracy
	for (int i = 0; i < 2; i++) {
		roots -= evalCubic(1.0, b, c, d, roots) / evalQuadratic(3.0, 2.0 * b, c, roots);
	}
	return bvec3(true);
}

bvec4 solveQuartic(in float a, in float b, in float c, in float d, in float e, inout vec4 roots) {
    // https://www.mdpi.com/2227-7390/10/14/2377
	// Solves Quartic Equation By Using The Method Given In The Above Paper
    float inva = 1.0 / a;
    float inva2 = inva * 0.5;
    float inva2a2 = inva2 * inva2;
    float bb = b * b;
    float p = -1.5 * bb * inva2a2 + c * inva;
    float q = bb * b * inva2a2 * inva2 - b * c * inva * inva2 + d * inva;
    float r = -0.1875 * bb * bb * inva2a2 * inva2a2 + 0.5 * c * bb * inva2a2 * inva2 - b * d * inva2a2 + e * inva;
	vec3 s = vec3(0.0);
    solveCubic(0.5 * -p, -r, 0.5 * p * r - 0.125 * q * q, s);
    float s2subp = 2.0 * s.x - p;
	if (s2subp < 0.0) {
		return bvec4(false);
	}
	float invs2subp = -2.0 * s.x - p;
	float sqrts2subp = sqrt(s2subp);
	float q2divsqrt = 2.0 * q / sqrts2subp;
	float invaddq2div = invs2subp + q2divsqrt;
	float invsubq2div = invs2subp - q2divsqrt;
	float bdiv4a = 0.25 * inva * b;
	bvec4 isReal = bvec4(false);
	if (invaddq2div >= 0.0) {
		roots.xy = 0.5 * (-sqrts2subp + vec2(1.0, -1.0) * sqrt(invaddq2div)) - bdiv4a;
		// Apply Newton Raphson Method To Increase The Accuracy
		roots.xy -= evalQuartic(a, b, c, d, e, roots.xy) / evalCubic(4.0 * a, 3.0 * b, 2.0 * c, d, roots.xy);
		isReal.xy = bvec2(true);
	}
	if (invsubq2div >= 0.0) {
		roots.zw = 0.5 * (sqrts2subp + vec2(1.0, -1.0) * sqrt(invsubq2div)) - bdiv4a;
		// Apply Newton Raphson Method To Increase The Accuracy
		roots.zw -= evalQuartic(a, b, c, d, e, roots.zw) / evalCubic(4.0 * a, 3.0 * b, 2.0 * c, d, roots.zw);
		isReal.zw = bvec2(true);
	}
	return isReal;
}

bool thritorius(in Ray ray, inout float hitdist, inout vec3 normal, inout material mat) {
	// Equation: x^2(x^2 + 2y^2 - 6y + 2z^2) + y^2(y^2 + 2y + 2z^2) + z^2(10z^2 - 12) + 1 = 0
	// Substitute Light Ray Equation Into This Equation To Get The Polynomial In Terms Of t, Substitution Has Been Done Manually, Then Solve For t Using The Quartic Equation Solver.
	if (!BoundingSphere(ray, vec3(0.0, 0.0, 0.0), 4.05)) {
		return false;
	}
	vec3 o = ray.origin.xzy;
	vec3 d = ray.dir.xzy;
	float a4 = dot(vec3(d.x, d.y, 10.0 * d.z), d * d * d) + 2.0 * dot(d.xyz * d.xyz, d.yzx * d.yzx);
	float a3 = 4.0 * (dot(vec3(o.x, o.y, 10.0 * o.z), d * d * d) + dot(o * d, vec3(dot(d.yz, d.yz), dot(d.xz, d.xz), dot(d.xy, d.xy)))) + 2.0 * d.y * d.y * d.y - 6.0 * d.x * d.x * d.y;
	float a2 = 6.0 * dot(vec3(o.x, o.y, 10.0 * o.z), o * d * d) + 2.0 * (dot(o.xy * o.xy, d.yx * d.yx) + dot(o.xz * o.xz, d.zx * d.zx) + dot(o.yz * o.yz, d.zy * d.zy)) + 8.0 * dot(o.xxy * d.xxy, o.yzz * d.yzz) + 6.0 * o.y * d.y * d.y - 6.0 * o.y * d.x * d.x - 12.0 * o.x * d.x * d.y - 12.0 * d.z * d.z;
	float a1 = 4.0 * (dot(vec3(o.x, o.y, 10.0 * o.z) * o * o, d) + dot(o.xx * o.xx, o.yz * d.yz) + dot(o.yy * o.yy, o.xz * d.xz) + dot(o.zz * o.zz, o.xy * d.xy)) + 6.0 * o.y * o.y * d.y - 6.0 * o.x * o.x * d.y - 12.0 * o.x * o.y * d.x - 24.0 * o.z * d.z;
	float a0 = dot(vec3(o.x, o.y, 10.0 * o.z), o * o * o) + 2.0 * dot(o.xxy * o.xxy, o.yzz * o.yzz) + 2.0 * o.y * o.y * o.y - 6.0 * o.x * o.x * o.y - 12.0 * o.z * o.z + 1.0;

	vec4 roots = vec4(0.0);
	bvec4 isReal = solveQuartic(a4, a3, a2, a1, a0, roots);

	float t = 1e6;
	for (int i = 0; i < 4; i++) {
		if (isReal[i]) {
			if ((roots[i] < t) && (roots[i] > 0.0)) {
				t = roots[i];
			}
		}
	}

	if (t < hitdist) {
		hitdist = t;
		// Find The Normals By Calculating The Unit Gradient Of The Equation, Gradient Has Been Written Manually Through The Equations
		// Unit Gradient = normalize(<∂f/∂x,∂f/∂y,∂f/∂z>)
		float x = o.x + d.x * t;
		float y = o.y + d.y * t;
		float z = o.z + d.z * t;
		normal.x = 4.0 * x * x * x + 4.0 * x * y * y - 12.0 * x * y + 4.0 * x * z * z;
		normal.y = 40.0 * z * z * z + 4.0 * x * x * z + 4.0 * y * y * z - 24.0 * z;
		normal.z = 4.0 * y * y * y + 4.0 * x * x * y - 6.0 * x * x + 6.0 * y * y + 4.0 * y * z * z;
		normal = normalize(normal);
		mat = GetMaterial(1);
		return true;
	}
	return false;
}

float SDF(in vec3 p) {
	float s1 = length(p) - 2.0;
	float s2 = length(vec3(p.x, p.y - 2.0, p.z)) - 1.4;
	float s3 = length(vec3(p.x - 0.5, p.y - 2.5, p.z - 1.0)) - 0.5;
	float s4 = length(vec3(p.x + 0.5, p.y - 2.5, p.z - 1.0)) - 0.5;
	return min(min(min(s1, s2), s3), s4);
}

float RayMarching(in Ray ray, inout float hitdist) {
	float t = 0.0;
	float omega = 1.2;
	float previousRadius = 0.0;
	float stepLength = 0.0;
	vec3 p = vec3(0.0);
	float heatmap = 0.0;
	vec3 invdir = 1.0 / ray.dir;
	box boundingBox;
	boundingBox.pos = vec3(0.0, 0.7, 0.0);
	boundingBox.size = vec3(4.0, 5.4, 4.0);
	for (int i = 0; i < 256; i++) {
		heatmap += i;
		p = fma(ray.dir, vec3(t), ray.origin);
		float radius = abs(SDF(p));
		if ((radius + previousRadius) < stepLength) {
			stepLength = stepLength / omega;
		} else {
			previousRadius = radius;
			stepLength = radius * omega;
			t += stepLength;
		}
		if (radius < 1e-5) {
			break;
		}
		if (!RayIntersectAABB(p, invdir, boundingBox)) {
			return heatmap;
		}
	}
	hitdist = t;
	return heatmap;
}

float Intersection(in Ray ray, inout vec3 normal, inout material mat) {
	// Finds The Ray-Intersection Of Every Object In The Scene
	float hitdist = 1e6;
	int offset = 0;
	// Iterate Over All The Spheres In The Scene
	for (int i = 0; i < numObjects[0]; i++) {
		sphere object;
		object.pos = vec3(objects[5*i+offset], objects[5*i+1+offset], objects[5*i+2+offset]);
		object.radius = objects[5*i+3+offset];
		object.materialID = int(objects[5*i+4+offset])-1;
		SphereIntersection(ray, object, hitdist, normal, mat);
	}
	offset += 5*numObjects[0];
	// Iterate Over All The Planes In The Scene
	for (int i = 0; i < numObjects[1]; i++) {
		plane object;
		object.pos = vec3(objects[4*i+offset], objects[4*i+1+offset], objects[4*i+2+offset]);
		object.materialID = int(objects[4*i+3+offset])-1;
		PlaneIntersection(ray, object, hitdist, normal, mat);
	}
	offset += 4*numObjects[1];
	// Iterate Over All The Cubes In The Scene
	for (int i = 0; i < numObjects[2]; i++) {
		box object;
		object.pos = vec3(objects[10*i+offset], objects[10*i+1+offset], objects[10*i+2+offset]);
		object.rotation = vec3(objects[10*i+3+offset], objects[10*i+4+offset], objects[10*i+5+offset]);
		object.size = vec3(objects[10*i+6+offset], objects[10*i+7+offset], objects[10*i+8+offset]);
		object.materialID = int(objects[10*i+9+offset])-1;
		if (!BoundingSphere(ray, object.pos, 0.25 * dot(object.size, object.size))) {
			continue;
		}
		BoxIntersection(ray, object, hitdist, normal, mat);
	}
	offset += 10*numObjects[2];
	// Iterate Over All The Lenses In The Scene
	for (int i = 0; i < numObjects[3]; i++) {
		lens object;
		object.pos = vec3(objects[11*i+offset], objects[11*i+1+offset], objects[11*i+2+offset]);
		object.rotation = vec3(objects[11*i+3+offset], objects[11*i+4+offset], objects[11*i+5+offset]);
		object.radius = objects[11*i+6+offset];
		object.focalLength = objects[11*i+7+offset];
		object.thickness = objects[11*i+8+offset];
		object.isConverging = bool(objects[11*i+9+offset]);
		object.materialID = int(objects[11*i+10+offset])-1;
		int isOutside = 1;
		if (!BoundingSphere(ray, object.pos, object.radius * object.radius)) {
			continue;
		}
		LensIntersection(ray, object, hitdist, normal, isOutside, mat);
	}
	offset += 11*numObjects[3];
	ray.origin -= vec3(1.0, 1.06, -7.0);
	thritorius(ray, hitdist, normal, mat);

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

float SampleSpectra(in float start, in float end, in float rand){
	// Uniform Inverted CDF For Sampling
	return (end - start) * rand + start;
}

float InvSpectraPDF(in float start, in float end){
	// Inverse Of Uniform PDF
	return end - start;
}

vec2 SampleUniformUnitDisk(inout uint seed) {
	// Samples Uniformly Distributed Random Points On Unit Disk
	float phi = 2.0 * PI * RandomFloat(seed);
	float d = sqrt(RandomFloat(seed));
	return d * vec2(cos(phi), sin(phi));
}

vec3 SampleUniformUnitSphere(inout uint seed){
	// Samples Uniformly Distributed Random Points On Unit Sphere
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

vec3 SampleCosineDirectionHemisphere(in vec3 normal, inout uint seed){
	// Generate Cosine Distributed Random Vectors Within Normals Hemisphere
	vec3 sumvector = normal + SampleUniformUnitSphere(seed);
	return normalize(sumvector);
}

float CosineDirectionPDF(in float costheta) {
	// Idea: Integrating Over Cosine PDF Must Give 1
	// Solution: Divide Cosine By Pi
	// Note: Integrating Equation Is Solid Angle. Must Insert PDF Inside Integrals To Normalize
	return costheta / PI;
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

float EvaluateBRDF(in float l, in vec3 inDir, in vec3 outDir, in vec3 normal, in material mat) {
	// Evaluate The BRDF
	// Lambertian BRDF For Diffuse Surface
	float diffuse = SpectralPowerDistribution(l, mat.reflection.x, mat.reflection.y, int(mat.reflection.z)) / PI;
	return diffuse;
}

vec3 SampleBRDF(in vec3 inDir, in vec3 normal, inout uint seed) {
	// Sample Directions Of BRDF
	vec3 outDir = SampleCosineDirectionHemisphere(normal, seed);
	return outDir;
}

float BRDFPDF(in vec3 outDir, in vec3 normal) {
	// PDF For Sampling Directions Of BRDF
	return CosineDirectionPDF(dot(outDir, normal));
}

float TraceRay(in float l, inout float rayradiance, inout Ray inRay, inout uint seed, out bool isTerminate) {
	// Traces A Ray Along The Given Origin And Direction Then Calculates Light Interactions
	float radiance = 0.0;
	vec3 normal = vec3(0.0);
	material mat;
	float hitdist = Intersection(inRay, normal, mat);
	Ray outRay = inRay;
	if (hitdist < MAXDIST) {
		// Calculate The Next Ray's Origin And Direction
		outRay.origin = fma(inRay.dir, vec3(hitdist), inRay.origin);
		outRay.dir = SampleBRDF(inRay.dir, normal, seed);
		float BRDFpdf = BRDFPDF(outRay.dir, normal);
		// If The Ray Hits The Light Source
		if (mat.emission.y > 0.0) {
			radiance = fma(Emit(l, mat), rayradiance, radiance);
			// Terminate The Path If The Ray Hits The Light Source
			isTerminate = true;
			return radiance;
		}
		// Evaluate The BRDF
		float costheta = dot(outRay.dir, normal);
		rayradiance *= EvaluateBRDF(l, inRay.dir, outRay.dir, normal, mat) * costheta / BRDFpdf;
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
		inRay = outRay;
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

void TracePathLens(in float l, inout Ray ray, in vec3 forwardDir) {
	// Trace The Path Through The BiConvex Lens
	lens object;
	object.radius = lensData.x;
	object.focalLength = lensData.y;
	object.thickness = lensData.z;
	object.isConverging = true;
	object.pos = cameraPos + forwardDir * lensData.w;
	object.rotation = vec3(0.0, 90.0 - cameraAngle.y, cameraAngle.x);
	object.materialID = 0;
	for (int i = 0; i < 2; i++) {
		float hitdist = 1e6;
		vec3 normal = vec3(0.0);
		int isOutside = 1;
		material mat;
		LensIntersection(ray, object, hitdist, normal, isOutside, mat);
		// Calculate The Refractive Index
		float n1 = 1.0;
		float n2 = 1.0;
		if (isOutside == 1) {
			n1 = 1.0;
			n2 = RefractiveIndexBK7Glass(l);
		} else {
			n1 = RefractiveIndexBK7Glass(l);
			n2 = 1.0;
		}
		float n12 = n1 / n2;
		// Wavelength Changes As Refractive Index Changes
		// lambda = lamda_0 / n
		l = l * n12;
		// Calculate The Next Ray's Origin And Direction
		ray.origin = fma(ray.dir, vec3(hitdist), ray.origin);
		ray.dir = refract(ray.dir, normal, n12);
	}
}

vec3 Scene(in uvec2 xy, in vec2 uv, in uint k) {
	uint seed = GenerateSeed(xy, k);
	// SSAA
	uv += vec2(2.0 * RandomFloat(seed) - 0.5, 2.0 * RandomFloat(seed) - 0.5) / resolution;

	// This Is A Simple Camera Made Up Of A BiConvex Lens And An Aperture
	// Ray Originates From The Pixel Of Camera Sensor
	// Then Passes Through The Area Of Aperture
	// Then It Passes Through A BiConvex Lens
	Ray ray;
	mat3 matrix = RotationMatrix(vec3(cameraAngle, 0.0));
	uv *= -cameraSize * 0.5;
	// Position Of Each Pixel On Sensor As Ray Origin
	ray.origin = cameraPos + (vec3(uv, 0.0) * matrix);
	// Generates Random Point On Aperture
	vec3 pointOnAperture = cameraPos + (vec3(0.5 * apertureSize * SampleUniformUnitDisk(seed), apertureDist) * matrix);
	// Compute Random Direction Which Passes Through The Area Of Aperture From Camera Sensor
	ray.dir = normalize(pointOnAperture - ray.origin);

	vec3 color = vec3(0.0);

	float l = SampleSpectra(390.0, 720.0, RandomFloat(seed));
	vec3 forwardDir = vec3(matrix[0][2], matrix[1][2], matrix[2][2]);
	// Ray Passes Through The Lens Here
	TracePathLens(l, ray, forwardDir);
	color += TracePath(l, ray, seed) * WaveToXYZ(l) * InvSpectraPDF(390.0, 720.0);
	//float hitdist = 1e5;
	//float heatmap = RayMarching(ray, hitdist);
	//color += heatmap;

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
			color = ((samples - 1) * texture(screenTexture, gl_FragCoord.xy / resolution).xyz + color) / samples;
		}
	}
}

void main() {
	uvec2 xy = uvec2(gl_FragCoord.xy);
	vec2 uv = ((2.0 * vec2(xy) - resolution) / resolution.y);

	vec3 color = vec3(0.0);
	for (uint i = 0; i < samplesPerFrame; i++) {
		color += Scene(xy, uv, i);
	}
	// Simulate Exposure Variance Depending On Aperture Size And ISO
	color *= apertureSize * apertureSize * ISO;
	Accumulate(color);

	Fragcolor = vec4(color, 1.0);
}