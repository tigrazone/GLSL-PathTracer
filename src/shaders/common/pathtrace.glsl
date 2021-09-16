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
vec3 DirectLight(in Ray r, in State state)
//-----------------------------------------------------------------------
{
    vec3 Li = vec3(0.0);
    vec3 surfacePos = state.fhp + state.normal * EPS;

    BsdfSampleRec bsdfSampleRec;

    // Environment Light
#ifdef ENVMAP
#ifndef CONSTANT_BG
    {
        vec3 color;
        vec4 dirPdf = EnvSample(color);
        vec3 lightDir = dirPdf.xyz;
        float lightPdf = dirPdf.w;

        Ray shadowRay = Ray(surfacePos, lightDir);
        bool inShadow = AnyHit(shadowRay, INFINITY - EPS);

        if (!inShadow)
        {
            bsdfSampleRec.f = DisneyEval(state, -r.direction, state.ffnormal, lightDir, bsdfSampleRec.pdf);

            if (bsdfSampleRec.pdf > 0.0)
            {
                float misWeight = powerHeuristic(lightPdf, bsdfSampleRec.pdf);
                if (misWeight > 0.0)
                    Li += misWeight * bsdfSampleRec.f * abs(dot(lightDir, state.ffnormal)) * color / lightPdf;
            }
        }
    }
#endif
#endif

    // Analytic Lights 
#ifdef LIGHTS
    {
        LightSampleRec lightSampleRec;
        Light light;

        //Pick a light to sample
        int indexx = int(rand() * float(numOfLights)) * 8;

        // Fetch light Data
        vec3 position = texelFetch(lightsTex, ivec2(indexx, 0), 0).xyz;
        vec3 emission = texelFetch(lightsTex, ivec2(indexx + 1, 0), 0).xyz;
        vec3 u        = texelFetch(lightsTex, ivec2(indexx + 2, 0), 0).xyz; // u vector for rect
        vec3 v        = texelFetch(lightsTex, ivec2(indexx + 3, 0), 0).xyz; // v vector for rect
        vec3 nrm      = texelFetch(lightsTex, ivec2(indexx + 4, 0), 0).xyz; // v vector for rect
        vec3 uu       = texelFetch(lightsTex, ivec2(indexx + 5, 0), 0).xyz; // uu precalc for rect
        vec3 vv       = texelFetch(lightsTex, ivec2(indexx + 6, 0), 0).xyz; // vv precalc for rect
        vec3 params   = texelFetch(lightsTex, ivec2(indexx + 7, 0), 0).xyz;
        float radius  = params.x;
        float area    = params.y;
        float type    = params.z; // 0->rect, 1->sphere, 2->distant

        light = Light(position, emission, u, v, nrm, uu, vv, radius, area, type);
        sampleLight(light, lightSampleRec, surfacePos);
		
		vec3 lightDir;
		
		if(light.area > 0.0) {
			lightDir = lightSampleRec.surfacePos - surfacePos;
		} else {
			lightDir = light.u;
		}

        if (dot(lightDir, lightSampleRec.normal) < 0.0)
        {			
			float lightDist = INFINITY;
			if(light.area > 0.0) {
				lightDist = length(lightDir);
				lightDir *= 1.0f / lightDist;
			}
		
            Ray shadowRay = Ray(surfacePos, lightDir);
            bool inShadow = AnyHit(shadowRay, lightDist - EPS);

            if (!inShadow)
            {
                bsdfSampleRec.f = DisneyEval(state, -r.direction, state.ffnormal, lightDir, bsdfSampleRec.pdf);

                if (bsdfSampleRec.pdf > 0.0) {
					if(light.area > 0.0) {
						float lightPdf = - (lightDist * lightDist) / (light.area * dot(lightDir, lightSampleRec.normal));
						Li += powerHeuristic(lightPdf, bsdfSampleRec.pdf) * bsdfSampleRec.f * abs(dot(state.ffnormal, lightDir)) * lightSampleRec.emission / lightPdf;
					} else {
							Li += bsdfSampleRec.f * abs(dot(state.ffnormal, lightDir)) * lightSampleRec.emission;
					}
				}
            }
        }
    }
#endif

    return Li;
}


//-----------------------------------------------------------------------
vec3 PathTrace(Ray r)
//-----------------------------------------------------------------------
{
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);
    State state;
    LightSampleRec lightSampleRec;
    BsdfSampleRec bsdfSampleRec;
    vec3 absorption = vec3(0.0);
	
    state.specularBounce = false;
    
    for (int depth = 0; depth < maxDepth; depth++)
    {
        state.depth = depth;
        float t = ClosestHit(r, state, lightSampleRec);

        if (t == INFINITY)
        {
#ifdef CONSTANT_BG
            radiance += bgColor * throughput;
#else
#ifdef ENVMAP
            {
                float misWeight = 1.0f;
				float phi = hdrRotate + PI + atan(r.direction.z, r.direction.x);
				if(phi < 0.0f) phi += TWO_PI;
				if(phi > TWO_PI) phi -= TWO_PI;
				float theta = hdrRotateY + acos(r.direction.y);
				if(theta < 0.0f) theta += TWO_PI;
				if(theta > TWO_PI) theta -= TWO_PI;
                vec2 uv = vec2(phi * INV_TWO_PI,  theta* INV_PI);

                if (depth > 0 && !state.specularBounce)
                {
                    // TODO: Fix NaNs when using certain HDRs
                    float lightPdf = EnvPdf(r, uv);
                    misWeight = powerHeuristic(bsdfSampleRec.pdf, lightPdf);
                }
                radiance += misWeight * texture(hdrTex, uv).xyz * throughput * hdrMultiplier;
            }
#endif
#endif
            return radiance;
        }

        GetNormalsAndTexCoord(state, r);
        GetMaterialsAndTextures(state, r);

        radiance += state.mat.emission * throughput;

#ifdef LIGHTS
        if (state.isEmitter)
        {
            radiance += EmitterSample(r, state, lightSampleRec, bsdfSampleRec) * throughput;
            break;
        }
#endif

        // Reset absorption when ray is going out of surface
        if (state.isInside) {
            absorption = vec3(0.0);
		}
		else 
		{
			// Add absoption
			throughput *= exp(-absorption * t);
		}

        radiance += DirectLight(r, state) * throughput;

        bsdfSampleRec.f = DisneySample(state, -r.direction, state.ffnormal, bsdfSampleRec.L, bsdfSampleRec.pdf);

		float dotL = dot(state.ffnormal, bsdfSampleRec.L);

        // Set absorption only if the ray is currently inside the object.
        if (dotL < 0.0) {
            absorption = state.mat.extinction1;
			dotL = - dotL;
		}

        if (bsdfSampleRec.pdf > 0.0)
            throughput *= bsdfSampleRec.f * dotL / bsdfSampleRec.pdf;
        else
            break;

#ifdef RR
        // Russian roulette
        if (depth >= RR_DEPTH)
        {
            float q = min(max(throughput.x, max(throughput.y, throughput.z)) + 0.001, 0.95);
            if (rand() > q)
                break;
            throughput /= q;
        }
#endif

        r.direction = bsdfSampleRec.L;
        r.origin = state.fhp + r.direction * EPS;
    }

    return radiance;
}