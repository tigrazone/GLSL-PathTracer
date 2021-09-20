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


//-----------------------------------------------------------------------
void Onb(in vec3 N, out vec3 T, out vec3 B)
//-----------------------------------------------------------------------
{
	float sgn = (N.z > 0.0f) ? 1.0f : -1.0f;
	float aa = - 1.0f / (sgn + N.z);
	float bb = N.x * N.y * aa;	
	
	T = vec3(1.0f + sgn * N.x * N.x * aa, sgn * bb, -sgn * N.x);
	B = vec3(bb, sgn + N.y * N.y * aa, -N.y);
}


 //----------------------------------------------------------------------
vec3 ImportanceSampleGTR1(float rgh, float r1, float r2)
//----------------------------------------------------------------------
{
    float a = max(0.001, rgh);
    float a2 = a * a;

    float phi = r1 * TWO_PI;

    float cosTheta = sqrt((1.0 - pow(a2, 1.0 - r1)) / (1.0 - a2));
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    return vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

//----------------------------------------------------------------------
vec3 ImportanceSampleGTR2_aniso(float ax, float ay, float r1, float r2)
//----------------------------------------------------------------------
{
    float phi = r1 * TWO_PI;

    float sinPhi = ay * sin(phi);
    float cosPhi = ax * cos(phi);
    float tanTheta = sqrt(r2 / (1 - r2));

    return vec3(tanTheta * cosPhi, tanTheta * sinPhi, 1.0);
}

//----------------------------------------------------------------------
vec3 ImportanceSampleGTR2(float rgh, float r1, float r2)
//----------------------------------------------------------------------
{
    float a = max(0.001, rgh);

    float phi = r1 * TWO_PI;

    float cosTheta = sqrt((1.0 - r2) / (1.0 + (a * a - 1.0) * r2));
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    return vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}


// Eric Heitz. Sampling the GGX Distribution of Visible Normals
// http://jcgt.org/published/0007/04/01/
vec3 GGXVNDF_Sample(float r1, float r2, vec3 n, float rgh, vec3 incoming)
{
	float alpha = rgh*rgh;
    // Section 3.2: transforming the view direction to the hemisphere configuration
    vec3 Vh = normalize(vec3(alpha * incoming.x, alpha * incoming.y, incoming.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0 ? (vec3(-Vh.y, Vh.x, 0) / sqrt(lensq)) : (vec3(1, 0, 0));
    vec3 T2 = cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r = sqrt(r2);
    float phi = TWO_PI * r1;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;
    // Section 4.3: reprojection onto hemisphere
    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    vec3 Ne = normalize(vec3(alpha * Nh.x, alpha * Nh.y, max(0.0f, Nh.z)));
    return Ne;
}

//-----------------------------------------------------------------------
float SchlickFresnel(float u)
//-----------------------------------------------------------------------
{
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

//-----------------------------------------------------------------------
float DielectricFresnel(float cos_theta_i, float eta)
//-----------------------------------------------------------------------
{
    float sinThetaTSq = eta * eta * (1.0f - cos_theta_i * cos_theta_i);

    // Total internal reflection
    if (sinThetaTSq > 1.0)
        return 1.0;

    float cos_theta_t = sqrt(max(1.0 - sinThetaTSq, 0.0));

    float rs = (eta * cos_theta_t - cos_theta_i) / (eta * cos_theta_t + cos_theta_i);
    float rp = (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);

    return 0.5f * (rs * rs + rp * rp);
}

//-----------------------------------------------------------------------
float GTR1(float NDotH, float a)
//-----------------------------------------------------------------------
{
    if (a >= 1.0)
        return INV_PI;
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

//-----------------------------------------------------------------------
float GTR2(float NDotH, float a)
//-----------------------------------------------------------------------
{
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;
    return a2 / (PI * t * t);
}

//-----------------------------------------------------------------------
float GTR2_aniso(float NDotH, float HDotX, float HDotY, float ax, float ay)
//-----------------------------------------------------------------------
{
    float a = HDotX / ax;
    float b = HDotY / ay;
    float c = a * a + b * b + NDotH * NDotH;
    return 1.0 / (PI * ax * ay * c * c);
}

//-----------------------------------------------------------------------
float SmithG_GGX(float NDotV, float alphaG)
//-----------------------------------------------------------------------
{
    float a = alphaG * alphaG;
    float b = NDotV * NDotV;
    return 1.0 / (NDotV + sqrt(a + b - a * b));
}

//-----------------------------------------------------------------------
float SmithG_GGX_aniso(float NDotV, float VDotX, float VDotY, float ax, float ay)
//-----------------------------------------------------------------------
{
    float a = VDotX * ax;
    float b = VDotY * ay;
    float c = NDotV;
    return 1.0 / (NDotV + sqrt(a * a + b * b + c * c));
}

//-----------------------------------------------------------------------
vec3 CosineSampleHemisphere(float r1, float r2)
//-----------------------------------------------------------------------
{
    vec3 dir;
    float r = sqrt(r1);
    float phi = TWO_PI * r2;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));

    return dir;
}

//-----------------------------------------------------------------------
vec3 UniformSampleHemisphere(float r1, float r2)
//-----------------------------------------------------------------------
{
    float r = sqrt(max(0.0, 1.0 - r1 * r1));
    float phi = TWO_PI * r2;

    return vec3(r * cos(phi), r * sin(phi), r1);
}

//-----------------------------------------------------------------------
vec3 UniformSampleSphere()
//-----------------------------------------------------------------------
{
    float r1 = rand();
    float r2 = rand();

    float z = 1.0 - r1 - r1;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = TWO_PI * r2;

    return vec3(r * cos(phi), r * sin(phi), z);
}

//-----------------------------------------------------------------------
float powerHeuristic(float a, float b)
//-----------------------------------------------------------------------
{
    float t = a * a;
    return t / (b * b + t);
}

//-----------------------------------------------------------------------
void sampleSphereLight(in Light light, inout LightSampleRec lightSampleRec, in vec3 surfacePos)
//-----------------------------------------------------------------------
{	
    float r1 = rand();
    float r2 = rand();
	vec3 sphereCentertoSurface = normalize(surfacePos - light.position);
	vec3 sampledDir = UniformSampleHemisphere(r1, r2);
    vec3 T, B;
    Onb(sphereCentertoSurface, T, B);
    sampledDir = T * sampledDir.x + B * sampledDir.y + sphereCentertoSurface * sampledDir.z;
	
    lightSampleRec.normal = normalize(sampledDir);
    lightSampleRec.surfacePos = light.position + lightSampleRec.normal * light.radius;
    lightSampleRec.emission = light.emission * float(numOfLights);
}

//-----------------------------------------------------------------------
void sampleRectLight(in Light light, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
    float r1 = rand();
    float r2 = rand();

    lightSampleRec.surfacePos = light.position + light.u * r1 + light.v * r2;
    lightSampleRec.normal = light.nrm;
    lightSampleRec.emission = light.emission * float(numOfLights);
}


//-----------------------------------------------------------------------
void sampleDistantLight(in Light light, inout LightSampleRec lightSampleRec, in vec3 surfacePos)
//-----------------------------------------------------------------------
{
    lightSampleRec.normal = normalize(surfacePos - light.position);
    lightSampleRec.emission = light.emission * float(numOfLights);
}

//-----------------------------------------------------------------------
void sampleLight(in Light light, inout LightSampleRec lightSampleRec, in vec3 surfacePos)
//-----------------------------------------------------------------------
{
	int itype = int(light.type);
    if (itype == QUAD_LIGHT)
        sampleRectLight(light, lightSampleRec);
    else
	   if (itype == SPHERE_LIGHT)
        sampleSphereLight(light, lightSampleRec, surfacePos);
	else
	   if (itype == DISTANT_LIGHT)
        sampleDistantLight(light, lightSampleRec, surfacePos);
}


#ifdef ENVMAP
#ifndef CONSTANT_BG

//-----------------------------------------------------------------------
float EnvPdf(in Ray r, in vec2 uv)
//-----------------------------------------------------------------------
{
    float pdf = texture(hdrCondDistTex, uv).y * texture(hdrMarginalDistTex, vec2(uv.y, 0.)).y;
    return (pdf * hdrResolution) / (TWO_PI_PI * sqrt(1.0f - r.direction.y * r.direction.y));
}

//-----------------------------------------------------------------------
vec4 EnvSample(inout vec3 color)
//-----------------------------------------------------------------------
{
    float r1 = rand();
    float r2 = rand();

    float v = texture(hdrMarginalDistTex, vec2(r1, 0.)).x;
    float u = texture(hdrCondDistTex, vec2(r2, v)).x;

    color = texture(hdrTex, vec2(u, v)).xyz * hdrMultiplier;
    float theta = v * PI;
	
	float sinTheta = - sin(theta);

    if (sinTheta == 0.0)
	{
		return vec4(0.0f, 1.0f, 0.0f, 0.0f);
	}
	
    float phi = u * TWO_PI;
	
    float pdf = texture(hdrCondDistTex, vec2(u, v)).y * texture(hdrMarginalDistTex, vec2(v, 0.)).y;

    return vec4(sinTheta * cos(phi), cos(theta), sinTheta * sin(phi), - (pdf * hdrResolution) / (TWO_PI_PI * sinTheta));
}

#endif
#endif

//-----------------------------------------------------------------------
vec3 EmitterSample(in Ray r, in State state, in LightSampleRec lightSampleRec, in BsdfSampleRec bsdfSampleRec)
//-----------------------------------------------------------------------
{
    vec3 Le;

    if (state.depth == 0 || state.specularBounce)
        Le = lightSampleRec.emission;
    else
        Le = powerHeuristic(bsdfSampleRec.pdf, lightSampleRec.pdf) * lightSampleRec.emission;

    return Le;
}


//-----------------------------------------------------------------------
void GetNormalsAndTexCoord(inout State state, inout Ray r)
//-----------------------------------------------------------------------
{
    vec4 n1 = texelFetch(normalsTex, state.triID.x);
    vec4 n2 = texelFetch(normalsTex, state.triID.y);
    vec4 n3 = texelFetch(normalsTex, state.triID.z);

    vec2 t1 = vec2(tempTexCoords.x, n1.w);
    vec2 t2 = vec2(tempTexCoords.y, n2.w);
    vec2 t3 = vec2(tempTexCoords.z, n3.w);

    state.texCoord = t1 * state.bary.x + t2 * state.bary.y + t3 * state.bary.z;

    vec3 normal = (n1.xyz * state.bary.x + n2.xyz * state.bary.y + n3.xyz * state.bary.z);

    mat3 normalMatrix = transpose(inverse(mat3(transform)));
   
    state.normal = normalize(normalMatrix * normal);
}

//-----------------------------------------------------------------------
void GetMaterialsAndTextures(inout State state, in Ray r)
//-----------------------------------------------------------------------
{
    int index7 = state.matID * 8;
    Material mat;

    vec4 param1 = texelFetch(materialsTex, ivec2(index7, 0), 0);
    vec4 param2 = texelFetch(materialsTex, ivec2(index7 + 1, 0), 0);
    vec4 param3 = texelFetch(materialsTex, ivec2(index7 + 2, 0), 0);
    vec4 param4 = texelFetch(materialsTex, ivec2(index7 + 3, 0), 0);
    vec4 param5 = texelFetch(materialsTex, ivec2(index7 + 4, 0), 0);
    vec4 param6 = texelFetch(materialsTex, ivec2(index7 + 5, 0), 0);
    vec4 param7 = texelFetch(materialsTex, ivec2(index7 + 6, 0), 0);
    vec4 param8 = texelFetch(materialsTex, ivec2(index7 + 7, 0), 0);

    mat.albedo         = param1.xyz;
    mat.specular       = param1.w;

    mat.emission       = param2.xyz;
    mat.anisotropic    = param2.w;

    mat.metallic       = param3.x;
    mat.roughness      = max(param3.y, 0.001);

    mat.subsurface     = param3.z;
    mat.specularTint   = param3.w;

    mat.sheen          = param4.x;
    mat.sheenTint      = param4.y;
    mat.clearcoat      = param4.z;
    mat.clearcoatGloss = param4.w;

    mat.specTrans      = param5.x;
    mat.ior            = param5.y;
    mat.atDistance     = param5.z;

    mat.extinction     = param6.xyz;

    mat.texIDs         = param7.xyz;

    mat.extinction1    = param8.xyz;
	
    vec2 texUV = state.texCoord;
    texUV.y = 1.0 - texUV.y;

    // Albedo Map
    if (int(mat.texIDs.x) >= 0)
        mat.albedo *= pow(texture(textureMapsArrayTex, vec3(texUV, int(mat.texIDs.x))).xyz, vec3(2.2));

    // Metallic Roughness Map
    if (int(mat.texIDs.y) >= 0)
    {
        vec2 matRgh;
        // TODO: Change metallic roughness maps in repo to linear space and remove gamma correction
        matRgh = pow(texture(textureMapsArrayTex, vec3(texUV, int(mat.texIDs.y))).xy, vec2(2.2));
        mat.metallic = matRgh.x;
        mat.roughness = max(matRgh.y, 0.001);
    }

    // Normal Map
    if (int(mat.texIDs.z) >= 0)
    {
        vec3 nrm = normalize(2.0 * texture(textureMapsArrayTex, vec3(texUV, int(mat.texIDs.z))).xyz - 1.0);
				
        // Orthonormal Basis
        vec3 T, B;
		
		Onb(state.normal, T, B);

        nrm = T * nrm.x + B * nrm.y + state.normal * nrm.z;
        state.normal = normalize(nrm);
    }
	
	state.isInside = dot(state.normal, r.direction) <= 0.0;
	
    state.ffnormal = state.isInside ? state.normal : -state.normal;
	Onb(state.normal, state.tangent, state.bitangent);

    // Calculate anisotropic roughness along the tangent and bitangent directions
	//because not used anisotropy yet
	/*
    float aspect = sqrt(1.0 - mat.anisotropic * 0.9);
    mat.ax = max(0.001, mat.roughness / aspect);
    mat.ay = max(0.001, mat.roughness * aspect);
	*/

    state.mat = mat;
    state.eta = state.isInside ? (1.0 / mat.ior) : mat.ior;
}