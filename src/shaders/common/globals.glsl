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

#define INFINITY  1e15
#define EPS 0.0001

#define QUAD_LIGHT 0
#define SPHERE_LIGHT 1
#define DISTANT_LIGHT 2

mat4 transform;

vec2 seed;
vec3 tempTexCoords;

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
    vec3 extinction1;
    vec3 texIDs;
    // Roughness calculated from anisotropic param
    float ax;
    float ay;
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
    float eta;
    float hitDist;

    vec3 fhp;
    vec3 normal;
    vec3 ffnormal;
    vec3 tangent;
    vec3 bitangent;

    bool isEmitter;
    bool isInside;
    bool specularBounce;

    vec2 texCoord;
    vec3 bary;
    ivec3 triID;
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
    vec3 surfacePos;
    vec3 normal;
    vec3 emission;
    float pdf;
};

uniform Camera camera;


float rand0()
{
    seed -= randomVector.xy;
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}


float one111 = 1.0f / float( 0xffffffffU );

#define hashi(x)   lowbias32(x)
#define hash(x)  ( float( hashi(x) ) * one111 )

uint lowbias32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}


float rand() { 
	seed -= randomVector.xy;
	return hash( floatBitsToUint(seed.x) + hashi(floatBitsToUint(seed.y)) );
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