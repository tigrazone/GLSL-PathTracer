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
bool ClosestHit(Ray r, inout State state, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
    float t = INFINITY;
    float d, dt;

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

        if (type == QUAD_LIGHT)
        {
            if (dot(normal, r.direction) > 0.) // Hide backfacing quad light
                continue;
			
            vec4 plane = vec4(normal, radius);
			
			float d = RectIntersect(position, uu, vv, plane, r, dt);
            if (d < 0.)
               continue;
            if (d < t)
            {
                t = d;
                lightSampleRec.pdf = -(t * t) / (area * dt);
                lightSampleRec.emission = emission;
                state.isEmitter = true;
            }
        }

        if (type == SPHERE_LIGHT)
        {
            d = SphereIntersect(u.x, position, r);
            if (d < 0.)
                continue;
            if (d < t)
            {
                t = d;
                vec3 hitPt = r.origin + t * r.direction;
                float cosTheta = dot(-r.direction, normalize(hitPt - position));
                // TODO: Fix this. Currently assumes the light will be hit only from the outside
                lightSampleRec.pdf = (t * t) / (area * cosTheta * 0.5);
                lightSampleRec.emission = emission;
                state.isEmitter = true;
            }
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

    int currMatID = 0;
    bool BLAS = false;

    ivec3 triID = ivec3(-1);
    mat4 transMat;
    mat4 transform;
    vec3 texCoords;
    vec3 bary;

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

                if (all(greaterThanEqual(uvt, vec4(0.0))) && uvt.z < t)
                {
                    t = uvt.z;
                    triID = vertIndices;
                    state.matID = currMatID;
                    bary = uvt.wxy;
                    texCoords = vec3(v0.w, v1.w, v2.w);
                    transform = transMat;
                }
            }
        }
        else if (leaf < 0) // Leaf node of TLAS
        {
			int leaf4 = (-leaf - 1) << 2;
            vec4 r1 = texelFetch(transformsTex, ivec2(leaf4 , 0), 0).xyzw;
            vec4 r2 = texelFetch(transformsTex, ivec2(leaf4 + 1, 0), 0).xyzw;
            vec4 r3 = texelFetch(transformsTex, ivec2(leaf4 + 2, 0), 0).xyzw;
            vec4 r4 = texelFetch(transformsTex, ivec2(leaf4 + 3, 0), 0).xyzw;

            transMat = mat4(r1, r2, r3, r4);

            rTrans.origin    = vec3(inverse(transMat) * vec4(r.origin, 1.0));
            rTrans.direction = vec3(inverse(transMat) * vec4(r.direction, 0.0));

            // Add a marker. We'll return to this spot after we've traversed the entire BLAS
            stack[ptr++] = -1;
            index = leftIndex;
            BLAS = true;
            currMatID = rightIndex;
            continue;
        }
        else
        {
            leftHit  = AABBIntersect(texelFetch(BVH, leftIndex  * 3).xyz, texelFetch(BVH, leftIndex  * 3 + 1).xyz, rTrans);
            rightHit = AABBIntersect(texelFetch(BVH, rightIndex * 3).xyz, texelFetch(BVH, rightIndex * 3 + 1).xyz, rTrans);

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
        }
    }

    // No intersections
    if (t == INFINITY)
        return false;

    state.hitDist = t;
    state.fhp = r.origin + r.direction * t;

    // Ray hit a triangle and not a light source
    if (triID.x != -1)
    {
        state.isEmitter = false;

        // Normals
        vec4 n1 = texelFetch(normalsTex, triID.x);
        vec4 n2 = texelFetch(normalsTex, triID.y);
        vec4 n3 = texelFetch(normalsTex, triID.z);

        // Create texcoords from w coord of vertices and normals
        vec2 t1 = vec2(texCoords.x, n1.w);
        vec2 t2 = vec2(texCoords.y, n2.w);
        vec2 t3 = vec2(texCoords.z, n3.w);

        state.texCoord = t1 * bary.x + t2 * bary.y + t3 * bary.z;

        vec3 normal = normalize(n1.xyz * bary.x + n2.xyz * bary.y + n3.xyz * bary.z);

        state.normal = normalize(transpose(inverse(mat3(transform))) * normal);
        state.ffnormal = dot(state.normal, r.direction) <= 0.0 ? state.normal : -state.normal;

        Onb(state.normal, state.tangent, state.bitangent);
    }

    return true;
}