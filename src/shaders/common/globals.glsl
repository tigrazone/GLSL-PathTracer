/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this softwareand associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define PI        3.14159265358979323846
#define TWO_PI    6.28318530717958648

#define INV_PI        0.31830988618379067153776752674503
#define INV_TWO_PI    0.15915494309189533576888376337251

#define TWO_PI_PI    19.739208802178717237668981999752

#define INFINITY  1e6
#define EPS 0.001

#define QUAD_LIGHT 0
#define SPHERE_LIGHT 1
#define DISTANT_LIGHT 2

struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct Material
{
    vec3 albedo;
    float specular;
    vec3 emission;
    float anisotropic;
    float metallic;
    float roughness;
    float subsurface;
    float specularTint;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;
    float specTrans;
    float ior;
    float atDistance;
    vec3 extinction;
    // Roughness calculated from anisotropic param
    float ax;
    float ay;	
	
    float roughness2;
    vec3 extinction1;
};

struct Camera
{
    vec3 up;
    vec3 right;
    vec3 forward;
    vec3 position;
    float fov;
    float fovTAN;
    float fovTAN1;
    float focalDist;
    float aperture;
};

struct Light
{
    vec3 position;
    vec3 emission;
    vec3 u;
    vec3 v;
	vec3 nrm;
	vec3 uu;
	vec3 vv;
    float radius;
    float area;
    float type;
};

struct State
{
    int depth;
    float eta, eta2;
    float hitDist;

    vec3 fhp;
    vec3 normal;
    vec3 ffnormal;
    vec3 tangent;
    vec3 bitangent;

    bool isEmitter;	
	bool isBackside;

    vec2 texCoord;
    int matID;
    Material mat;
};

struct BsdfSampleRec
{
    vec3 L;
    vec3 f;
    float pdf;
};

struct LightSampleRec
{
    vec3 normal;
    vec3 emission;
    vec3 direction;
    float dist;
    float pdf;
};

struct TriPrecalcData
{
	vec3 uu;
	vec3 vv;
	vec3 normal;
    float delta;
};

uniform Camera camera;

//RNG from code by Moroz Mykhailo (https://www.shadertoy.com/view/wltcRS)

//internal RNG state 
uvec4 seed;
ivec2 pixel;

#define oneOFbig 2.3283064370807973754314699618685e-10

void InitRNG(vec2 p, int frame)
{
    pixel = ivec2(p);
    seed = uvec4(p, uint(frame), uint(p.x) + uint(p.y));
}

void pcg4d(inout uvec4 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.w; v.y += v.z * v.x; v.z += v.x * v.y; v.w += v.y * v.z;
    v = v ^ (v >> 16u);
    v.x += v.y * v.w; v.y += v.z * v.x; v.z += v.x * v.y; v.w += v.y * v.z;
}

float rand()
{
    pcg4d(seed); 
	// return float(seed.x) / float(0xffffffffu);
	return float(seed.x) * oneOFbig;
}


//temporal conversion
vec3 rgb_to_ycocg(in vec3 colour) {
vec3 color4 = colour * 0.25f;
	return vec3(
		 color4.x + color4.y + color4.y + color4.z,
		 color4.x + color4.x - color4.z - color4.z,
		-color4.x + color4.y + color4.y - color4.z
	);
}