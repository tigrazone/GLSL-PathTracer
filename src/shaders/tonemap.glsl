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

#version 330

out vec4 color;
in vec2 TexCoords;

uniform sampler2D pathTraceTexture;
uniform float invSampleCounter;

vec4 ToneMap(in vec4 c, float limit)
{
    float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

    return c * 1.0 / (1.0 + luminance / limit);
}

vec4 ToneMapOPT(in vec4 c, float limit)
{
    float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

    return c / (1.0 + luminance * limit);
}

/*
original OpenCL version
float3 uc2TonemapFunc(float3 x)
{
    float A = 0.22; // shoulder strength
    float B = 0.30; // linear strength
    float C = 0.10; // linear angle
    float D = 0.20; // toe strength
    float E = 0.01; // toe numerator
    float F = 0.30; // tone denominator

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

float3 uncharted2Tonemap(float3 x)
{
    float W = 11.2; // linear white point

    float exposureBias = 2.0;
	float3 color = uc2TonemapFunc(exposureBias * x) / uc2TonemapFunc((float3)(W, W, W));
    return color;
}
*/

#define CB (0.03f)
#define DE (0.002f)
#define DF (0.06f)
#define ExF (0.033333333333333333333f)

vec3 uc2TonemapFunc(vec3 x)
{
    float A = 0.22; // shoulder strength
    float B = 0.30; // linear strength
    float C = 0.10; // linear angle
    float D = 0.20; // toe strength
    float E = 0.01; // toe numerator
    float F = 0.30; // tone denominator

    // return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    return ((x*(A*x+CB)+DE)/(x*(A*x+B)+DF))-ExF;
}

vec3 uncharted2Tonemap(vec3 x)
{
    float W = 11.2; // linear white point

    float exposureBias = 2.0;
	vec3 color = uc2TonemapFunc(exposureBias * x) / uc2TonemapFunc(vec3(W, W, W));
    return color;
}

vec3 reinhardTonemap(vec3 color)
{
    return color / (1.0f + color);
}

#define one2_2 0.454545454545454545f
#define one1_5 0.666666666666666666666666666f

#define tonemapClassicTonemap
//#define tonemapUncharted2
//#define tonemapRAW
//#define tonemapReinhard

void main()
{
    color = texture(pathTraceTexture, TexCoords) * invSampleCounter;
	
    //color = pow(ToneMap(color, 1.5), vec4(1.0 / 2.2));
#ifdef tonemapClassicTonemap	
    color = pow(ToneMapOPT(color, one1_5), vec4(one2_2));
#else
#ifdef tonemapUncharted2
    color.xyz = pow(uncharted2Tonemap(color.xyz), vec3(one2_2));
#else
#ifdef tonemapReinhard
    color.xyz = pow(reinhardTonemap(color.xyz), vec3(one2_2));
#endif
#else
#ifdef tonemapRAW
    color.xyz = pow(color.xyz, vec3(one2_2));
#endif
#endif
#endif
}