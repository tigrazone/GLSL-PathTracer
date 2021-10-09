/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
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

 /* References:
 * [1] https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf
 * [2] https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
 * [3] https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf
 * [4] https://github.com/mmacklin/tinsel/blob/master/src/disney.h
 * [5] http://simon-kallweit.me/rendercompo2015/report/
 * [6] http://shihchinw.github.io/2015/07/implementing-disney-principled-brdf-in-arnold.html
 * [7] https://github.com/mmp/pbrt-v4/blob/0ec29d1ec8754bddd9d667f0e80c4ff025c900ce/src/pbrt/bxdfs.cpp#L76-L286
 * [8] https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
 */

//-----------------------------------------------------------------------
vec3 EvalDielectricReflection(State state, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf, float dotVH)
//-----------------------------------------------------------------------
{	
    float dotNH = dot(N, H);
	
    float F = DielectricFresnel(dotVH, state.eta);
    float D = GTR2(dotNH, state.mat.roughness);
    
    pdf = D * dotNH * F / (4.0 * abs(dotVH));

    float G = SmithG_GGX((dot(N, L)), state.mat.roughness) * SmithG_GGX(abs(dot(N, V)), state.mat.roughness);
    return state.mat.albedo * F * D * G;
}

//-----------------------------------------------------------------------
vec3 EvalDielectricRefraction(State state, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf, float dotVH)
//-----------------------------------------------------------------------
{
    float dotNH = dot(N, H);
    float dotLH = dot(L, H);
	
    float F = DielectricFresnel(dotVH, state.eta);
    float D = GTR2(dotNH, state.mat.roughness);

    float denomSqrt = dotLH + dotVH * state.eta;
    float pdf0 = (1.0 - F) * D * abs(dotLH) / (denomSqrt * denomSqrt);
    pdf = dotNH * pdf0;

    float G = SmithG_GGX(abs(dot(N, L)), state.mat.roughness) * SmithG_GGX(abs(dot(N, V)), state.mat.roughness);
    return state.mat.albedo * pdf0 * G * abs(dotVH) * 4.0 * state.eta * state.eta;
}

//-----------------------------------------------------------------------
vec3 EvalSpecular(State state, vec3 Cspec0, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf, float dotVH)
//-----------------------------------------------------------------------
{
    float dotNH = dot(N, H);
    float D = GTR2(dotNH, state.mat.roughness);
    pdf = D * dotNH / (4.0 * dotVH);

    vec3 F = mix(Cspec0, vec3(1.0), SchlickFresnel(dot(L, H)));
    float G = SmithG_GGX((dot(N, L)), state.mat.roughness) * SmithG_GGX(abs(dot(N, V)), state.mat.roughness);
    return F * D * G;
}

//-----------------------------------------------------------------------
vec3 EvalClearcoat(State state, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf, float dotVH)
//-----------------------------------------------------------------------
{
    float dotNH = dot(N, H);
    float D = GTR1(dotNH, mix(0.1, 0.001, state.mat.clearcoatGloss));
    pdf = D * dotNH / (4.0 * dotVH);

    float FH = SchlickFresnel(dot(L, H));
    float F = mix(0.04, 1.0, FH);
    float G = SmithG_GGX(dot(N, L), 0.25) * SmithG_GGX(dot(N, V), 0.25);
    return vec3(0.25 * state.mat.clearcoat * F * D * G);
}

//-----------------------------------------------------------------------
vec3 EvalDiffuse(State state, vec3 Csheen, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf, float dotNL)
//-----------------------------------------------------------------------
{
	float dotNV = dot(N, V);
	
    pdf = dotNL * (INV_PI);

    // Diffuse
    float FL = SchlickFresnel(dotNL);
    float FV = SchlickFresnel(dotNV);

    // Fake Subsurface TODO: Replace with volumetric scattering
    float dotLH = dot(L, H);
	float Fss90 = dotLH * dotLH * state.mat.roughness;
	
    float Fd90 = 0.5 + Fss90 + Fss90;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);
	
	float fd1 = Fd;
    vec3 Fsheen = state.mat.sheen > 0.0 ? SchlickFresnel(dotLH) * state.mat.sheen * Csheen : vec3(0);
	
	if(state.mat.subsurface > 0.0) {		
		float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
		fd1 = mix(Fd, 1.25 * (Fss * (1.0 / (dotNL + dotNV) - 0.5) + 0.5), state.mat.subsurface);
	}
    return ((INV_PI) * fd1 * state.mat.albedo + Fsheen) * (1.0 - state.mat.metallic);
}

//-----------------------------------------------------------------------
vec3 EvalDiffuseSpecularClearcoat(State state, vec3 Csheen, vec3 Cspec0, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf, inout float pdfS, inout float pdfC, float dotNL)
//-----------------------------------------------------------------------
{
	float dotNV = dot(N, V);
    float dotLH = dot(L, H);
	
    pdf = dotNL * (INV_PI);

    // Diffuse
    float FL = SchlickFresnel(dotNL);
    float FV = SchlickFresnel(dotNV);
	float FH = SchlickFresnel(dotLH);

    // Fake Subsurface TODO: Replace with volumetric scattering
	float Fss90 = dotLH * dotLH * state.mat.roughness;
	
    float Fd90 = 0.5 + Fss90 + Fss90;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);
	
	float fd1 = Fd;
    vec3 Fsheen = state.mat.sheen > 0.0 ? FH * state.mat.sheen * Csheen : vec3(0);
	
	if(state.mat.subsurface > 0.0) {		
		float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
		fd1 = mix(Fd, 1.25 * (Fss * (1.0 / (dotNL + dotNV) - 0.5) + 0.5), state.mat.subsurface);
	}
    vec3 f = ((INV_PI) * fd1 * state.mat.albedo + Fsheen) * (1.0 - state.mat.metallic);	
	
	
	//specular
    float dotNH = dot(N, H);
	float pdfMul = dotNH / (4.0 * dot(V, H));
	
	/*
	pdfS = 0;
	if(state.mat.roughness > 0.0) {
		*/
		float D = GTR2(dotNH, state.mat.roughness);
	
		pdfS = D * pdfMul;

		vec3 Fs = mix(Cspec0, vec3(1.0), FH);
		float Gs = SmithG_GGX((dotNL), state.mat.roughness) * SmithG_GGX(abs(dotNV), state.mat.roughness);
		f += Fs * D * Gs;
	//}
	
	//clearcoat
    float Dc = GTR1(dotNH, mix(0.1, 0.001, state.mat.clearcoatGloss));
    pdfC = Dc * pdfMul;
	
	if(state.mat.clearcoat > 0.0) {
		float Fc = mix(0.04, 1.0, FH);
		float Gc = SmithG_GGX(dotNL, 0.25) * SmithG_GGX(dotNV, 0.25);
		return f + vec3(0.25 * state.mat.clearcoat * Fc * Dc * Gc);
	}
	
	return f;
}

//-----------------------------------------------------------------------
vec3 DisneySample(inout State state, vec3 V, vec3 N, inout vec3 L, inout float pdf)
//-----------------------------------------------------------------------
{
    pdf = 0.0;
    vec3 f = vec3(0.0);

    float r1 = rand();
    float r2 = rand();

    float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);
    float transWeight = (1.0 - state.mat.metallic) * state.mat.specTrans;

    vec3 Cdlin = state.mat.albedo;
    float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

    vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
	
	float dotNL;
	float dotVH;

    // TODO: Reuse random numbers and reduce so many calls to rand()
    if (rand() < transWeight)
    {
		// if(state.mat.roughness > 0.0) {
        vec3 H = ImportanceSampleGTR2(state.mat.roughness, r1, r2);
		//GGXVNDF_Sample(float r1, float r2, vec3 n, float rgh, vec3 incoming)
        //vec3 H = GGXVNDF_Sample(r1, r2, N, state.mat.roughness, V);
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
		
		dotVH = dot(V, H);

        if (dotVH < 0.0) {
            H = -H;
			dotVH = -dotVH;
		}

        vec3 R = reflect(-V, H);
        float F = DielectricFresnel(abs(dot(R, H)), state.eta);

        // Reflection/Total internal reflection
        if (rand() < F)
        {
            L = //normalize
			(R);			
			if (dot(N, L) > 0.0)
			{
				f = EvalDielectricReflection(state, V, N, L, H, pdf, dotVH);
			}
        }
        else // Transmission
        {
            L = //normalize
			(refract(-V, H, state.eta));
			if (dot(N, L) < 0.0)
			{
				f = EvalDielectricRefraction(state, V, N, L, H, pdf, dotVH);
			}
        }

        f *= transWeight;
        pdf *= transWeight;
		//}
    }
    else
    {
        if (rand() < diffuseRatio)
        { 
            L = CosineSampleHemisphere(r1, r2);
            L = state.tangent * L.x + state.bitangent * L.y + N * L.z;
			dotNL = dot(N, L); 
			if (dotNL > 0.0)
			{
				vec3 H = normalize(L + V);
				vec3 Csheen = mix(vec3(1.0), Ctint, state.mat.sheenTint);
				f = EvalDiffuse(state, Csheen, V, N, L, H, pdf, dotNL);
				pdf *= diffuseRatio;
			}
        }
        else // Specular
        {			
            float primarySpecRatio = 1.0 / (1.0 + state.mat.clearcoat);
            
            // Sample primary specular lobe
            if (rand() < primarySpecRatio) 
            {
				//if(state.mat.roughness > 0.0) {
					// TODO: Implement http://jcgt.org/published/0007/04/01/
					vec3 H = ImportanceSampleGTR2(state.mat.roughness, r1, r2);
					//vec3 H = GGXVNDF_Sample(r1, r2, N, state.mat.roughness, V);
					H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
					dotVH = dot(V, H);
					if (dotVH < 0.0) {
						H = -H;
						dotVH = -dotVH;
					}

					L = //normalize
					(reflect(-V, H));
					if (dot(N, L) > 0.0)
					{						
						vec3 Cspec0 = mix(state.mat.specular * 0.08 * mix(vec3(1.0), Ctint, state.mat.specularTint), Cdlin, state.mat.metallic);
						f = EvalSpecular(state, Cspec0, V, N, L, H, pdf, dotVH);
						pdf *= primarySpecRatio * (1.0 - diffuseRatio);
					}
				//}
            }
            else // Sample clearcoat lobe
            {
                vec3 H = ImportanceSampleGTR1(mix(0.1, 0.001, state.mat.clearcoatGloss), r1, r2);
                H = state.tangent * H.x + state.bitangent * H.y + N * H.z;

				dotVH = dot(V, H);
                if (dotVH < 0.0) {
                    H = -H;
					dotVH = -dotVH;
				}

                L = //normalize
				(reflect(-V, H));
				if (dot(N, L) > 0.0)
				{
					f = EvalClearcoat(state, V, N, L, H, pdf, dotVH);
					pdf *= (1.0 - primarySpecRatio) * (1.0 - diffuseRatio);
				}
            }
        }

        f *= (1.0 - transWeight);
        pdf *= (1.0 - transWeight);
    }
    return f;
}

//-----------------------------------------------------------------------
vec3 DisneyEval(State state, vec3 V, vec3 N, vec3 L, inout float pdf)
//-----------------------------------------------------------------------
{
    vec3 H;
	
	float dotNL = dot(N, L);
	
    bool refl = dotNL > 0.0;

    if (refl)
        H = normalize(L + V);
    else
        H = normalize(L + V * state.eta);
	
	float dotVH = dot(V, H);
    if (dotVH < 0.0) {
        H = -H;
		dotVH = -dotVH;
	}

    float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);
    float primarySpecRatio = 1.0 / (1.0 + state.mat.clearcoat);
    float transWeight = (1.0 - state.mat.metallic) * state.mat.specTrans;

    vec3 brdf = vec3(0.0);
    vec3 bsdf = vec3(0.0);
    float brdfPdf = 0.0;
    float bsdfPdf = 0.0;

    if (transWeight > 0.0
	//&& state.mat.roughness > 0.0
	)
    {
        // Reflection
        if (refl) 
        {
			bsdf = EvalDielectricReflection(state, V, N, L, H, bsdfPdf, dotVH); 
        }
        else // Transmission
        {
            bsdf = EvalDielectricRefraction(state, V, N, L, H, bsdfPdf, dotVH);
        }
    }

    float m_pdf;

    if (transWeight < 1.0 && refl)
    {
        vec3 Cdlin = state.mat.albedo;
        float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

        vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
        vec3 Cspec0 = mix(state.mat.specular * 0.08 * mix(vec3(1.0), Ctint, state.mat.specularTint), Cdlin, state.mat.metallic);
        vec3 Csheen = mix(vec3(1.0), Ctint, state.mat.sheenTint);

/*
        // Diffuse
        brdf += EvalDiffuse(state, Csheen, V, N, L, H, m_pdf);
        brdfPdf += m_pdf * diffuseRatio;
            
        // Specular
        brdf += EvalSpecular(state, Cspec0, V, N, L, H, m_pdf);
        brdfPdf += m_pdf * primarySpecRatio * (1.0 - diffuseRatio);
            
        // Clearcoat
        brdf += EvalClearcoat(state, V, N, L, H, m_pdf);
        brdfPdf += m_pdf * (1.0 - primarySpecRatio) * (1.0 - diffuseRatio);
*/		
		float m_pdfS, m_pdfC;

		brdf += EvalDiffuseSpecularClearcoat(state, Csheen, Cspec0, V, N, L, H, m_pdf, m_pdfS, m_pdfC, dotNL);
		brdfPdf += m_pdf * diffuseRatio;
        brdfPdf += ((m_pdfS - m_pdfC)* primarySpecRatio + m_pdfC) * (1.0 - diffuseRatio);

    }

    pdf = mix(brdfPdf, bsdfPdf, transWeight);
    return mix(brdf, bsdf, transWeight);
}
