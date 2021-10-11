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

#include <iostream>

#include "Scene.h"
#include "Camera.h"
#include "Utils.h"

#include <ctime>

namespace GLSLPT
{
    void Scene::AddCamera(Vec3 pos, Vec3 lookAt, float fov)
    {
        delete camera;
        camera = new Camera(pos, lookAt, fov);
    }

    int Scene::AddMesh(const std::string& filename)
    {
        int id = -1;
		
        // Check if mesh was already loaded		
		auto it = mesh_names.find(filename);
		
		if (it == mesh_names.end()) {			   
			id = meshes.size();
			mesh_names[filename] = id;
		} else {
			return it->second;
		}

        Mesh* mesh = new Mesh;
		
        printf("Loading %s ...\n", filename.c_str());
        if (mesh->LoadFromFile(filename))
        {
            meshes.push_back(mesh);
            printf("Model %s loaded\n", filename.c_str());
        }
        else
            id = -1;
        return id;
    }

    void Scene::AddTexture(const std::string& filename, float *usedObj)
    {
        int id = -1;
		Texture* texture;
		
        // Check if texture was already loaded
		auto it = tex_names.find(filename);
		
		if (it == tex_names.end()) {			   
			id = textures.size();
			tex_names[filename] = id;
			
			texture = new Texture;
			texture->name = filename;
			
			texture->usedTimes = 0;
			
			//printf("*tex id=%d %s\n", id, (texture->name).c_str());
			textures.push_back(texture);
		} else {
			id = it->second;
			texture = textures[id];
		}		
		
		*usedObj = id;
    }

    int Scene::AddMaterial(const Material& material)
    {
        int id = materials.size();
        materials.push_back(material);
        return id;
    }

    bool Scene::AddHDR(const std::string& filename)
    {
        struct stat stt;
	
		stat(filename.c_str(), &stt);
		
		if(hdrData != nullptr) {
			if(filename == HDRfn) {	
	
				if(stt.st_dev == HDRst.st_dev &&
				   stt.st_size == HDRst.st_size &&
				   stt.st_ctime == HDRst.st_ctime
				   )
				   {
					printf("Reuse HDR %s\n", filename.c_str());
					return renderOptions.useEnvMap;
				   }
			}
		}
		
		delete hdrData;
		
		clock_t time1, time2;
		
		time1 = clock();
		printf("HDR loading %s ...\n", filename.c_str());
		
        hdrData = HDRLoader::load(filename.c_str());
        if (hdrData == nullptr) {
            printf("Unable to load HDR\n");
            renderOptions.useEnvMap = false;
        }
        else
        {
            printf("HDR %s loaded\n", filename.c_str());
            renderOptions.useEnvMap = true;
			HDRfn = filename;
			HDRst = stt;
        }
		
		time2 = clock();
		
		show_elapsed_time(time2, time1);
		
		return renderOptions.useEnvMap;
    }

    bool Scene::AddEXR(const std::string& filename)
    {
        struct stat stt;
	
		stat(filename.c_str(), &stt);		
		
		if(hdrData != nullptr) {
			if(filename == HDRfn) {	
	
				if(stt.st_dev == HDRst.st_dev &&
				   stt.st_size == HDRst.st_size &&
				   stt.st_ctime == HDRst.st_ctime
				   )
				   {
					printf("Reuse EXR %s\n", filename.c_str());
					return renderOptions.useEnvMap;
				   }
			}
		}		
		
		delete hdrData;
		
		clock_t time1, time2;
		
		time1 = clock();
		printf("EXR loading %s ...\n", filename.c_str());
		
        hdrData = EXRLoader::load(filename.c_str());
        if (hdrData == nullptr) {
            printf("Unable to load EXR\n");
            renderOptions.useEnvMap = false;
        }
        else
        {
            printf("EXR %s loaded\n", filename.c_str());
            renderOptions.useEnvMap = true;
			HDRfn = filename;
			HDRst = stt;
        }
		
		time2 = clock();
		
		show_elapsed_time(time2, time1);
		
		return renderOptions.useEnvMap;
    }

    int Scene::AddMeshInstance(const MeshInstance &meshInstance)
    {
        int id = meshInstances.size();
        meshInstances.push_back(meshInstance);
        return id;
    }

    int Scene::AddLight(const Light &light)
    {
        int id = lights.size();
        lights.push_back(light);
        return id;
    }

    void Scene::createTLAS()
    {
        // Loop through all the mesh Instances and build a Top Level BVH
        std::vector<RadeonRays::bbox> bounds;
        bounds.resize(meshInstances.size());
		
		printf("mesh Instances %ld\n", meshInstances.size());

        #pragma omp parallel for
        for (int i = 0; i < meshInstances.size(); i++)
        {
            RadeonRays::bbox bbox = meshes[meshInstances[i].meshID]->bvh->Bounds();
            Mat4 matrix = meshInstances[i].transform;

            Vec3 minBound = bbox.pmin;
            Vec3 maxBound = bbox.pmax;

            Vec3 right       = Vec3(matrix[0][0], matrix[0][1], matrix[0][2]);
            Vec3 up          = Vec3(matrix[1][0], matrix[1][1], matrix[1][2]);
            Vec3 forward     = Vec3(matrix[2][0], matrix[2][1], matrix[2][2]);
            Vec3 translation = Vec3(matrix[3][0], matrix[3][1], matrix[3][2]);

            Vec3 xa = right * minBound.x;
            Vec3 xb = right * maxBound.x;

            Vec3 ya = up * minBound.y;
            Vec3 yb = up * maxBound.y;

            Vec3 za = forward * minBound.z;
            Vec3 zb = forward * maxBound.z;

            minBound = Vec3::Min(xa, xb) + Vec3::Min(ya, yb) + Vec3::Min(za, zb) + translation;
            maxBound = Vec3::Max(xa, xb) + Vec3::Max(ya, yb) + Vec3::Max(za, zb) + translation;

            RadeonRays::bbox bound;
            bound.pmin = minBound;
            bound.pmax = maxBound;

            bounds[i] = bound;
        }
        sceneBvh->Build(&bounds[0], bounds.size());
        sceneBounds = sceneBvh->Bounds();
    }

    void Scene::createBLAS()
    {
		totalTris = 0;
		
        // Loop through all meshes and build BVHs
        #pragma omp parallel for
        for (int i = 0; i < meshes.size(); i++)
        {
            printf("Building BVH for %s\n", meshes[i]->name.c_str());
			meshes[i]->tris = meshes[i]->BuildBVH();
            totalTris += meshes[i]->tris;
			
        }
		
		if(totalTris > 10005) {
			float sz, sz1;
			char kb_mega_giga;
			
			sz = totalTris;
			convert_mega_giga(sz, sz1, kb_mega_giga);
			
			printf("Total %.2f%c triangles loaded\n", sz1, kb_mega_giga);
		}
		else {
			printf("Total %d triangles loaded\n", totalTris);
		}
    }
    
    void Scene::RebuildInstances()
    {
		float atD1;
        delete sceneBvh;
        sceneBvh = new RadeonRays::Bvh(10.0f, 64, false);

        createTLAS();
        bvhTranslator.UpdateTLAS(sceneBvh, meshInstances);

        //Copy transforms
        for (int i = 0; i < meshInstances.size(); i++)
            transforms[i] = meshInstances[i].transform;
		
		//precalc extinction
        for (int i = 0; i < materials.size(); i++) {
			if(materials[i].atDistance > 0.0) {
				// -log(state.mat.extinction) / state.mat.atDistance
				atD1 = 1.0f / materials[i].atDistance;
				materials[i].extinction1.x = -log(materials[i].extinction.x) * atD1;
				materials[i].extinction1.y = -log(materials[i].extinction.y) * atD1;
				materials[i].extinction1.z = -log(materials[i].extinction.z) * atD1;
			}
		}

        instancesModified = true;
    }

    void Scene::CreateAccelerationStructures()
    {
		clock_t time1, time2;
		
		time1 = clock();
		
        createBLAS();

        printf("Building scene BVH\n");
        createTLAS();

        // Flatten BVH
        bvhTranslator.Process(sceneBvh, meshes, meshInstances);

        int verticesCnt = 0;
		
		//set true to used materials
		materialUsed.resize(materials.size(), false);		
        for (int i = 0; i < meshInstances.size(); i++)
        {
			materialUsed[meshInstances[i].materialID] = true;
		}
		
		emissiveMaterials.clear();
		
		//calculate textures usage
		for (int j = 0; j < materials.size(); j++) {
			if(materialUsed[j]) {
				if (materials[j].albedoTexID > -1.0f) {
					textures[(int)materials[j].albedoTexID]->usedTimes ++;
				}
				
				if (materials[j].metallicRoughnessTexID > -1.0f) {
					textures[(int)materials[j].metallicRoughnessTexID]->usedTimes ++;
				}
				
				if (materials[j].normalmapTexID > -1.0f) {
					textures[(int)materials[j].normalmapTexID]->usedTimes ++;
				}
				
				if (Vec3::Dot(materials[j].emission, materials[j].emission) > 0.0f) {
					emissiveMaterials[j] = 0;
				}
            }
		}

        //Copy mesh data
        for (int i = 0; i < meshes.size(); i++)
        {
            // Copy indices from BVH and not from Mesh
            int numIndices = meshes[i]->bvh->GetNumIndices();
            const int * triIndices = meshes[i]->bvh->GetIndices();

            for (int j = 0; j < numIndices; j++)
            {
                int index = triIndices[j];
                int index3 = index + index + index;
                int v1 = (index3) + verticesCnt;
                int v2 = (index3 + 1) + verticesCnt;
                int v3 = (index3 + 2) + verticesCnt;

                vertIndices.push_back(Indices{ v1, v2, v3 });
            }

            verticesUVX.insert(verticesUVX.end(), meshes[i]->verticesUVX.begin(), meshes[i]->verticesUVX.end());
            normalsUVY.insert(normalsUVY.end(), meshes[i]->normalsUVY.begin(), meshes[i]->normalsUVY.end());

            verticesCnt += meshes[i]->verticesUVX.size();
        }		
		
		meshLightTris = 0;
		unsigned long mesh_tri_sz;

        //Copy transforms
        transforms.resize(meshInstances.size());
        #pragma omp parallel for
        for (int i = 0; i < meshInstances.size(); i++) {
            transforms[i] = meshInstances[i].transform;
			if(emissiveMaterials.find(meshInstances[i].materialID) != emissiveMaterials.end()) {
				mesh_tri_sz = meshes[meshInstances[i].meshID]->tris;
				emissiveMaterials[meshInstances[i].materialID] += mesh_tri_sz;
				meshLightTris += mesh_tri_sz;
				meshLights.push_back(i);
			}
			
		}

		//find widow textures
		texWrongId = textures.size();
        
        printf("* textures=%d\n", texWrongId);
		int i;
        
		for (i = 0; i < texWrongId; i++)
        {
			if(! textures[i]->usedTimes) {
				texWrongId--;
				std::swap(textures[i], textures[texWrongId]);
				i--;
			}
		}

        printf("* widow textures=%d\n", textures.size() - texWrongId);
        

		//load textures in parallel
        #pragma omp parallel for
		for (i = 0; i < texWrongId; i++) {
			textures[i]->LoadTexture();
		}
		
        int notLoadedTex = 0;

		//find not loaded textures
		for (i = 0; i < texWrongId; i++)
        {
			if(textures[i]->texData == nullptr) {
                auto it = tex_names.find(textures[i]->name);
                int tex_id = it->second;

                notLoadedTex++;

                for (int j = 0; j < materials.size(); j++) {
                    if (((int)materials[j].albedoTexID) == tex_id) {
                        materials[j].albedoTexID = -1.0f;
                        printf("%d albedoTexID NOT loaded tex\n", j);
                    }
                    if (((int)materials[j].metallicRoughnessTexID) == tex_id) {
                        materials[j].metallicRoughnessTexID = -1.0f;
                        printf("%d metallicRoughnessTexID NOT loaded tex\n", j);
                    }
                    if (((int)materials[j].normalmapTexID) == tex_id) {
                        materials[j].normalmapTexID = -1.0f;
                        printf("%d normalmapTexID NOT loaded tex\n", j);
                    }
                }

                texWrongId--;
                std::swap(textures[i], textures[texWrongId]);
                i--;
			}
		}        

        printf("notLoadedTex %d\n", notLoadedTex);
        printf("*textures %d\n", texWrongId);
        
        //Copy Textures
        for (i = 0; i < texWrongId; i++)
        {
            texWidth = textures[i]->width;
            texHeight = textures[i]->height;
            int texSize = texWidth * texHeight;
            textureMapsArray.insert(textureMapsArray.end(), &textures[i]->texData[0], &textures[i]->texData[texSize * 3]);
        }
		
		time2 = clock();
		
		show_elapsed_time(time2, time1);
		
		printf("=============================\nLights: %d\n", lights.size());
		printf("Mesh lights: %d\nEmissive triangles: ", meshLights.size());
		
		float sz, sz1;
		char kb_mega_giga;
		
		if(meshLightTris > 10005) {
			
			sz = meshLightTris;
			convert_mega_giga(sz, sz1, kb_mega_giga);
			
			printf("%.2f%c\n", sz1, kb_mega_giga);
		}
		else {
			printf("%d\n", meshLightTris);
		}
		
		printf("-----------------------------\n");
		
		sz = sizeof(Vec4) * verticesUVX.size();
		convert_mega_giga(sz, sz1, kb_mega_giga);
		
		printf("verticesUVX mem: %.2f%c\n", sz1, kb_mega_giga);
		
		sz = sizeof(Vec4) * normalsUVY.size();
		convert_mega_giga(sz, sz1, kb_mega_giga);
		
		printf("normalsUVY mem: %.2f%c\n", sz1, kb_mega_giga);
		
		sz = sizeof(RadeonRays::BvhTranslator::Node) * bvhTranslator.nodes.size();
		convert_mega_giga(sz, sz1, kb_mega_giga);
		
		printf("bvh mem: %.2f%c  NODES: %d\n", sz1, kb_mega_giga, bvhTranslator.nodes.size());
		
		printf("-----------------------------\n");		
    }
}