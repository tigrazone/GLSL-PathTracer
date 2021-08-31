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

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Mesh.h"
#include <iostream>

#include "miniply-loader.h"

namespace GLSLPT
{
    bool Mesh::LoadFromFile(const std::string &filename)
    {
		clock_t time1, time2;
		
		time1 = clock();
		
        name = filename;
		
		char *subString, *p;
		std::string format = "obj";
						
		subString = strrchr((char*) name.c_str(), '.');
		p = subString;
		for ( ; *p; ++p) *p = tolower(*p);
		if(p - subString > 1) {
			format.assign(subString + 1);
		}
		
		if(format == "obj")
		{
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string err;
			bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), 0, true);

			if (!ret)
			{
				printf("Unable to load model\n");
				return false;
			}

			// Loop over shapes
			for (size_t s = 0; s < shapes.size(); s++) 
			{
				// Loop over faces(polygon)
				size_t index_offset = 0;

				for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
				{
					int fv = shapes[s].mesh.num_face_vertices[f];
					// Loop over vertices in the face.
					for (size_t v = 0; v < fv; v++)
					{
						// access to vertex
						tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
						
						size_t idx3 = idx.vertex_index + idx.vertex_index + idx.vertex_index;
						tinyobj::real_t vx = attrib.vertices[idx3];
						tinyobj::real_t vy = attrib.vertices[idx3 + 1];
						tinyobj::real_t vz = attrib.vertices[idx3 + 2];
						
						idx3 = idx.normal_index + idx.normal_index + idx.normal_index;
						tinyobj::real_t nx = attrib.normals[idx3];
						tinyobj::real_t ny = attrib.normals[idx3 + 1];
						tinyobj::real_t nz = attrib.normals[idx3 + 2];

						tinyobj::real_t tx, ty;
						
						// temporary fix
						if (!attrib.texcoords.empty())
						{
							tx = attrib.texcoords[2 * idx.texcoord_index];
							ty = attrib.texcoords[2 * idx.texcoord_index + 1];
						}
						else
						{
							tx = ty = 0;
						}

						verticesUVX.push_back(Vec4(vx, vy, vz, tx));
						normalsUVY.push_back(Vec4(nx, ny, nz, ty));
					}
		
					index_offset += fv;
				}
			}
		} else
		if(format == "ply") {
			
			TriMesh* trimesh = parse_file_with_miniply(filename.c_str(), false);
			bool ok = trimesh != nullptr;

			Vec3 v1, v2, v3, nrm;
			int idx, idx3;
			int f;
			float tx, ty;
			float tys[3], txs[3];
			bool calc_normal;
			
			if(ok) {
				printf("numIndices=%d\n", trimesh->numIndices);
				printf("numVerts=%d\n", trimesh->numVerts);
				printf("pos=%d\n", trimesh->pos != nullptr);
				printf("normal=%d\n", trimesh->normal != nullptr);
				printf("uv=%d\n", trimesh->uv != nullptr);
				
				//faces
				if(trimesh->numIndices) 
				{
					for (f = 0; f < trimesh->numIndices; f += 3) 
					{
						calc_normal = false;
						
						//vertex 1
						idx = trimesh->indices[f];
						idx3 = idx + idx +idx;
					
						v1.x = trimesh->pos[idx3];
						v1.y = trimesh->pos[idx3+1];
						v1.z = trimesh->pos[idx3+2];
						
						if(trimesh->uv != nullptr) {
							tx = trimesh->uv[idx + idx];
							ty = trimesh->uv[idx + idx + 1];
						} else {
							tx = 0;
							ty = 0;
						}
						
						verticesUVX.push_back(Vec4(v1.x, v1.y, v1.z, tx));					
						
						if(trimesh->normal != nullptr) {
							nrm.x = trimesh->normal[idx3];
							nrm.y = trimesh->normal[idx3+1];
							nrm.z = trimesh->normal[idx3+2];
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						} else {
							calc_normal = true;
							tys[0] = ty;
						}
						
						//vertex 2
						idx = trimesh->indices[f+1];
						idx3 = idx + idx +idx;
					
						v2.x = trimesh->pos[idx3];
						v2.y = trimesh->pos[idx3+1];
						v2.z = trimesh->pos[idx3+2];
						
						if(trimesh->uv != nullptr) {
							tx = trimesh->uv[idx + idx];
							ty = trimesh->uv[idx + idx + 1];
						} else {
							tx = 0;
							ty = 0;
						}
						
						verticesUVX.push_back(Vec4(v2.x, v2.y, v2.z, tx));					
						
						if(!calc_normal) {
							nrm.x = trimesh->normal[idx3];
							nrm.y = trimesh->normal[idx3+1];
							nrm.z = trimesh->normal[idx3+2];
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						} else {
							tys[1] = ty;
						}
						
						//vertex 3
						idx = trimesh->indices[f+2];
						idx3 = idx + idx +idx;
					
						v3.x = trimesh->pos[idx3];
						v3.y = trimesh->pos[idx3+1];
						v3.z = trimesh->pos[idx3+2];
						
						if(trimesh->uv != nullptr) {
							tx = trimesh->uv[idx + idx];
							ty = trimesh->uv[idx + idx + 1];
						} else {
							tx = 0;
							ty = 0;
						}
						
						verticesUVX.push_back(Vec4(v3.x, v3.y, v3.z, tx));					
						
						if(!calc_normal) {
							nrm.x = trimesh->normal[idx3];
							nrm.y = trimesh->normal[idx3+1];
							nrm.z = trimesh->normal[idx3+2];
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						} else {
							nrm = v2 - v1;
							v2 = v3 - v1;

							nrm = Vec3::Cross(nrm, v2);
							nrm = Vec3::Normalize(nrm);
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, tys[0]));
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, tys[1]));
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						}
					}
				} else //vertices
				{
					for (f = 0; f < trimesh->numVerts; f += 4)
					{
						calc_normal = false;
						
						//vertex 1
						idx = f;
						idx3 = idx + idx +idx;
					
						v1.x = trimesh->pos[idx3];
						v1.y = trimesh->pos[idx3+1];
						v1.z = trimesh->pos[idx3+2];
						
						if(trimesh->uv != nullptr) {
							tx = trimesh->uv[idx + idx];
							ty = trimesh->uv[idx + idx + 1];
						} else {
							tx = 0;
							ty = 0;
						}
						
						verticesUVX.push_back(Vec4(v1.x, v1.y, v1.z, tx));
						txs[0] = tx;
						
						if(trimesh->normal != nullptr) {
							nrm.x = trimesh->normal[idx3];
							nrm.y = trimesh->normal[idx3+1];
							nrm.z = trimesh->normal[idx3+2];
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						} else {
							calc_normal = true;
							tys[0] = ty;
						}
						
						//vertex 2
						idx = f+1;
						idx3 = idx + idx +idx;
					
						v2.x = trimesh->pos[idx3];
						v2.y = trimesh->pos[idx3+1];
						v2.z = trimesh->pos[idx3+2];
						
						if(trimesh->uv != nullptr) {
							tx = trimesh->uv[idx + idx];
							ty = trimesh->uv[idx + idx + 1];
						} else {
							tx = 0;
							ty = 0;
						}
						
						verticesUVX.push_back(Vec4(v2.x, v2.y, v2.z, tx));
						txs[1] = tx;
						
						if(!calc_normal) {
							nrm.x = trimesh->normal[idx3];
							nrm.y = trimesh->normal[idx3+1];
							nrm.z = trimesh->normal[idx3+2];
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						} else {
							tys[1] = ty;
						}
						
						//vertex 3
						idx = f+2;
						idx3 = idx + idx +idx;
					
						v3.x = trimesh->pos[idx3];
						v3.y = trimesh->pos[idx3+1];
						v3.z = trimesh->pos[idx3+2];
						
						if(trimesh->uv != nullptr) {
							tx = trimesh->uv[idx + idx];
							ty = trimesh->uv[idx + idx + 1];
						} else {
							tx = 0;
							ty = 0;
						}
						
						verticesUVX.push_back(Vec4(v3.x, v3.y, v3.z, tx));
						txs[2] = tx;
						
						if(!calc_normal) {
							nrm.x = trimesh->normal[idx3];
							nrm.y = trimesh->normal[idx3+1];
							nrm.z = trimesh->normal[idx3+2];
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						} else {
							nrm = v2 - v1;
							v2 = v3 - v1;
							
							nrm = Vec3::Cross(nrm, v2);
							nrm = Vec3::Normalize(nrm);
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, tys[0]));
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, tys[1]));
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						}						
						
						//vertex 4
						idx = f+3;
						if(idx >= trimesh->numVerts) continue;
						
						idx3 = idx + idx +idx;
						
						//CDB
						v1 = v3;
						v3 = v2;
						txs[0] = txs[2];
						tys[0] = tys[2];
						
						v2.x = trimesh->pos[idx3];
						v2.y = trimesh->pos[idx3+1];
						v2.z = trimesh->pos[idx3+2];
						
						if(trimesh->uv != nullptr) {
							tx = trimesh->uv[idx + idx];
							ty = trimesh->uv[idx + idx + 1];
						} else {
							tx = 0;
							ty = 0;
						}
						nrm.x = tx;
						nrm.y = ty;
						
						tx = txs[1];
						ty = tys[1];						
						
						txs[1] = nrm.x;
						tys[1] = nrm.y;
						
						verticesUVX.push_back(Vec4(v1.x, v1.y, v1.z, txs[0]));					
						verticesUVX.push_back(Vec4(v2.x, v2.y, v2.z, txs[1]));					
						verticesUVX.push_back(Vec4(v3.x, v3.y, v3.z, tx));					
						
						if(!calc_normal) {
							nrm.x = trimesh->normal[idx3];
							nrm.y = trimesh->normal[idx3+1];
							nrm.z = trimesh->normal[idx3+2];
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						} else {
							nrm = v2 - v1;
							v2 = v3 - v1;
							
							nrm = Vec3::Cross(nrm, v2);
							nrm = Vec3::Normalize(nrm);
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, tys[0]));
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, tys[1]));
							normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, ty));
						}
					}
				} //vertices end processing
			}
			else {
				printf("Unable to load model\n");
				return false;
			}
		}
		
		time2 = clock();
		printf("%.1fs\n", (float)(time2-time1)/(float)CLOCKS_PER_SEC);

        return true;
    }

    size_t Mesh::BuildBVH()
    {		
		clock_t time1, time2;
		
		time1 = clock();
		
        const size_t numTris = verticesUVX.size() / 3;
		
		printf("%ld tris\n", numTris);
		
        std::vector<RadeonRays::bbox> bounds(numTris);
		
		size_t i3 = 0;

        #pragma omp parallel for
        for (int i = 0; i < numTris; ++i)
        {
            const Vec3 v1 = Vec3(verticesUVX[i3]);
            const Vec3 v2 = Vec3(verticesUVX[i3 + 1]);
            const Vec3 v3 = Vec3(verticesUVX[i3 + 2]);

            bounds[i].grow(v1);
            bounds[i].grow(v2);
            bounds[i].grow(v3);
			
			i3 += 3;
        }

        bvh->Build(&bounds[0], numTris);		
		
		time2 = clock();
		printf("%.1fs\n", (float)(time2-time1)/(float)CLOCKS_PER_SEC);

        return numTris;
    }
}