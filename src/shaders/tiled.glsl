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
#define TILED

precision highp float;
precision highp int;
precision highp sampler2D;
precision highp samplerCube;
precision highp isampler2D;
precision highp sampler2DArray;

out vec4 color;
in vec2 TexCoords;
uniform int doAAA;

#include common/uniforms.glsl
#include common/globals.glsl
#include common/intersection.glsl
#include common/sampling.glsl
#include common/anyhit.glsl
#include common/closest_hit.glsl
#include common/disney.glsl
#include common/pathtrace.glsl


float r1, r2, cam_r1, cam_r2;
vec2 coordsTile, jitter, d;
vec3 rayDir, focalPoint, randomAperturePos, finalRayDir;
Ray ray;

void createNewCamRay()
{
    r1 = rand();
    r2 = rand();
	
    jitter.x = r1 < 1.0 ? sqrt(r1 + r1) - 1.0 : 1.0 - sqrt(2.0 - r1 - r1);
    jitter.y = r2 < 1.0 ? sqrt(r2 + r2) - 1.0 : 1.0 - sqrt(2.0 - r2 - r2);

    jitter *= screenResolution1;
    d = coordsTile + jitter;

    d.y *= camera.fovTAN1;
    d.x *= camera.fovTAN;
    rayDir = normalize(d.x * camera.right + d.y * camera.up + camera.forward);

    focalPoint = camera.focalDist * rayDir;
    randomAperturePos = vec3(0);
	
#ifdef CAMERA_APERTURE
    cam_r1 = rand() * TWO_PI;
    cam_r2 = rand() * camera.aperture;
    randomAperturePos = (cos(cam_r1) * camera.right + sin(cam_r1) * camera.up) * sqrt(cam_r2);
#endif
	
    finalRayDir = normalize(focalPoint - randomAperturePos);

    ray = Ray(camera.position + randomAperturePos, finalRayDir);
}


void main(void)
{    
    vec2 coordsFS;
	coordsTile = TexCoords;	
	
	vec2 invNumTiles = vec2(invNumTilesX, invNumTilesY);

	vec2 invNumTilesTILE = invNumTiles * vec2(float(tileX), float(tileY));
	
	vec2 xyoffset = invNumTilesTILE + invNumTilesTILE - vec2(1.0f, 1.0f);
	
	coordsTile = mix(xyoffset, xyoffset + invNumTiles + invNumTiles, coordsTile);
	coordsFS = mix(invNumTilesTILE, invNumTilesTILE + invNumTiles, TexCoords);
	
    seed = coordsFS;
	
	createNewCamRay();
    vec3 pixelColor = PathTrace(ray);
	float extraSPP = 1.0;

	vec4 accumed = texture(accumTexture, coordsFS);
    vec3 accumColor = accumed.xyz;
    float accumSPP = accumed.w;

    if (isCameraMoving) {
        accumColor = vec3(0.);
		accumSPP = 0;
		}
	
#ifdef AAA

#define aaa_nbPRCmin 0.3
#define aaa_nbPRCmax 0.6

int pass, i, j, pxls, neightbors, startXY, endXY, dNeightbors;
vec3 accumColorNear, sumPixelsValue;
vec2 startPos, nowPos;
float accumNearSPP, ddd3210, ddd321, dddNear;

//dont do aaa for first frame
if(sampleCounter > 1.0f) {
		vec3 accumColor0 = (accumColor + pixelColor) *(1.0f/(accumSPP+1.0f));
		float ddd2 = dot(accumColor0, accumColor0);
		
		float dist = aaa_minDist;
		float delta_dist = (aaa_maxDist - aaa_minDist) / float(AAA-1);
		
		float nbPRC = aaa_nbPRCmax;
		float nbPRC_dist = (aaa_nbPRCmax - aaa_nbPRCmin) / float(AAA-1);
		
		neightbors = AAA % 3;
		if(neightbors == 0) neightbors = 3;
		dNeightbors = -1;
		
	for(pass=0; pass < AAA; pass++) {
		//look at neighbor pixels
		
		if(neightbors==0 || neightbors==4) {
			dNeightbors = -dNeightbors;
			neightbors = 2;			
		}
		
		startXY = -neightbors;
		endXY = neightbors+1;
		startPos = coordsFS - invScreenResolution*neightbors;
		
		pxls = 0;
		sumPixelsValue = vec3(0);

		for(j=startXY;j<endXY;j++) {
			nowPos = startPos;
			for(i=startXY;i<endXY;i++) {
				if(i==0 && j==0) continue;				
				if(nowPos.x<0 || nowPos.x>=1 || nowPos.y<0 || nowPos.y>=1) continue;
				
				pxls++;
				accumed = texture(accumTexture, nowPos);
				vec3 accumColorNear = accumed.xyz;
				float accumNearSPP = accumed.w;
				
				sumPixelsValue += accumNearSPP > 1 ? accumColorNear/accumNearSPP : accumColorNear;
				
				nowPos += vec2(invScreenResolution.x, 0);
			}
			startPos += vec2(0, invScreenResolution.y);
		}
		
		if(pxls>0) {
			sumPixelsValue *= nbPRC/pxls;
			sumPixelsValue += pixelColor * ((1.0f - nbPRC)/extraSPP);
		} else {
			sumPixelsValue = pixelColor * (1.0f/extraSPP);
		}
		
		dddNear = dot(sumPixelsValue, sumPixelsValue);
		
		ddd3210 = ddd2;
		
		if(dddNear > ddd3210) {
			float temp = ddd3210;
			ddd3210 = dddNear;
			dddNear = temp;
		}
		
		ddd321 = dddNear / ddd3210;
		
		if(ddd321 > dist) 
		{
			createNewCamRay();
			vec3 accumColor1 = PathTrace(ray);
			
			pixelColor += accumColor1;
			accumSPP += 1.0f;
			extraSPP += 1.0f;
			
			ddd2 = dddNear;
		}
		
		dist += delta_dist;
		
		neightbors += dNeightbors;
		nbPRC -= nbPRC_dist;
	}
}
#endif
	
	accumSPP += 1.0f;
    color = vec4(pixelColor + accumColor,accumSPP);
}