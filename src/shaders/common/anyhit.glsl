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
bool AnyHit(Ray r, float maxDist)
//-----------------------------------------------------------------------
{
#ifdef LIGHTS
    // Intersect Emitters
    for (int i = 0; i < numOfLights; i++)
    {
        // Fetch light Data
		int idx = i * 8;
        vec3 position = texelFetch(lightsTex, ivec2(idx , 0), 0).xyz;
        vec3 emission = texelFetch(lightsTex, ivec2(idx + 1, 0), 0).xyz;
        vec3 u        = texelFetch(lightsTex, ivec2(idx + 2, 0), 0).xyz;
        vec3 v        = texelFetch(lightsTex, ivec2(idx + 3, 0), 0).xyz;
        vec3 normal   = texelFetch(lightsTex, ivec2(idx + 4, 0), 0).xyz;
        vec3 uu 	  = texelFetch(lightsTex, ivec2(idx + 5, 0), 0).xyz;
        vec3 vv 	  = texelFetch(lightsTex, ivec2(idx + 6, 0), 0).xyz;
        vec3 params   = texelFetch(lightsTex, ivec2(idx + 7, 0), 0).xyz;
        float radius  = params.x;
        float area    = params.y;
        int type    = int(params.z);

        // Intersect rectangular area light
        if (type == QUAD_LIGHT)
        {
            vec4 plane = vec4(normal, radius);
			
			float d = RectIntersect(position, uu, vv, plane, r);
            if (d > 0.0 && d < maxDist)
                return true;
        }

        // Intersect spherical area light
        if (type == SPHERE_LIGHT)
        {
            float d = SphereIntersect(u.x, position, r);
            if (d > 0.0 && d < maxDist)
                return true;
        }
    }
#endif

    // Intersect BVH and tris
    int stack[64];
    int ptr = 0;
    stack[ptr++] = -1;

    int index = topBVHIndex;
    float leftHit = 0.0;
    float rightHit = 0.0;

    bool BLAS = false;

    Ray rTrans;
    rTrans.origin = r.origin;
    rTrans.direction = r.direction;

    while (index != -1)
    {
        ivec3 LRLeaf = ivec3(texelFetch(BVH, index * 3 + 2).xyz);

        int leftIndex  = int(LRLeaf.x);
        int rightIndex = int(LRLeaf.y);
        int leaf       = int(LRLeaf.z);

        if (leaf > 0) // Leaf node of BLAS
        {
            for (int i = 0; i < rightIndex; i++) // Loop through tris
            {
                ivec3 vertIndices = ivec3(texelFetch(vertexIndicesTex, leftIndex + i).xyz);

                vec4 v0 = texelFetch(verticesTex, vertIndices.x);
				vec4 v1 = texelFetch(verticesTex, vertIndices.y);
                vec4 v2 = texelFetch(verticesTex, vertIndices.z);
				
				
                vec3 e0 = v1.xyz - v0.xyz;
                vec3 e1 = v2.xyz - v0.xyz;
                vec3 pv = cross(rTrans.direction, e1);
                float det = dot(e0, pv);

                vec3 tv = rTrans.origin - v0.xyz;
                vec3 qv = cross(tv, e0);

                vec4 uvt;
                uvt.x = dot(tv, pv);
                uvt.y = dot(rTrans.direction, qv);
                uvt.z = dot(e1, qv);
                uvt.xyz *= 1.0f / det;
                uvt.w = 1.0 - uvt.x - uvt.y;

                if (all(greaterThanEqual(uvt, vec4(0.0))) && uvt.z < maxDist)
                    return true;
								
				/*
				// Fetch triPrecalc Data
				int idxtri = int(vertIndices.x) * 3;
				vec4 nrm_d 	= texelFetch(triPrecalcsTex, ivec2(idxtri , 0), 0);
				vec4 uu1 	= texelFetch(triPrecalcsTex, ivec2(idxtri + 1, 0), 0);
				vec4 vv1 	= texelFetch(triPrecalcsTex, ivec2(idxtri + 2 , 0), 0);
				vec3 uu = uu1.xyz;
				vec3 vv = vv1.xyz;
				
				//vec4 plane = vec4(normal, radius);			
				//float d = RectIntersect(position, uu, vv, plane, r);
				vec3 n = nrm_d.xyz;
				float dt = dot(rTrans.direction, n);
				float t = (nrm_d.w - dot(n, rTrans.origin)) / dt;
				//6* 1/
				if (t > EPS)
				{
					vec3 p = rTrans.origin + rTrans.direction * t;
					vec3 vi = p - v0;
					float a1 = dot(uu, vi);
					if (a1 >= 0.)
					{
						float a2 = dot(vv, vi);
						//9* => 15* 1/
						if (a2 >= 0. && a1 + a2 <= 1.)
							return true;
					}
				}
				*/
            }
        }
        else if (leaf < 0) // Leaf node of TLAS
        {
			int leaf4 = (-leaf - 1) << 2;
            vec4 r1 = texelFetch(transformsTex, ivec2(leaf4 , 0), 0).xyzw;
            vec4 r2 = texelFetch(transformsTex, ivec2(leaf4 + 1, 0), 0).xyzw;
            vec4 r3 = texelFetch(transformsTex, ivec2(leaf4 + 2, 0), 0).xyzw;
            vec4 r4 = texelFetch(transformsTex, ivec2(leaf4 + 3, 0), 0).xyzw;

            mat4 transform = mat4(r1, r2, r3, r4);

            rTrans.origin    = vec3(inverse(transform) * vec4(r.origin, 1.0));
            rTrans.direction = vec3(inverse(transform) * vec4(r.direction, 0.0));

            // Add a marker. We'll return to this spot after we've traversed the entire BLAS
            stack[ptr++] = -1;

            index = leftIndex;
            BLAS = true;
            continue;
        }
        else
        {
            leftHit =  AABBIntersect(texelFetch(BVH, leftIndex  * 3 ).xyz, texelFetch(BVH, leftIndex  * 3 + 1).xyz, rTrans);
            rightHit = AABBIntersect(texelFetch(BVH, rightIndex * 3 ).xyz, texelFetch(BVH, rightIndex * 3 + 1).xyz, rTrans);

            if (leftHit > 0.0 && rightHit > 0.0)
            {
                int deferred = -1;
                if (leftHit > rightHit)
                {
                    index = rightIndex;
                    deferred = leftIndex;
                }
                else
                {
                    index = leftIndex;
                    deferred = rightIndex;
                }

                stack[ptr++] = deferred;
                continue;
            }
            else if (leftHit > 0.)
            {
                index = leftIndex;
                continue;
            }
            else if (rightHit > 0.)
            {
                index = rightIndex;
                continue;
            }
        }
        index = stack[--ptr];

        // If we've traversed the entire BLAS then switch to back to TLAS and resume where we left off
        if (BLAS && index == -1)
        {
            BLAS = false;

            index = stack[--ptr];

            rTrans.origin = r.origin;
            rTrans.direction = r.direction;
            continue;
        }
    }

    return false;
}