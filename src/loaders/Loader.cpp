/*

Copyright (c) 2018 Miles Macklin

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

*/

/* 
    This is a modified version of the original code 
    Link to original code: https://github.com/mmacklin/tinsel
*/

#include "Utils.h"
#include "Loader.h"
#include <tiny_obj_loader.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <unordered_map>
#include <stdio.h>

#include <ctime>

#include "ImGuizmo.h"

namespace GLSLPT
{
    static const int kMaxLineLength = 2048;
    int(*Log)(const char* szFormat, ...) = printf;
	

    bool LoadSceneFromObjFile(const std::string &filename, Scene *scene, RenderOptions& renderOptions)
    {		
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string err;
			std::string path = filename.substr(0, filename.find_last_of("/\\")) + "/";
			
			printf("MTL path = %s\n", path.c_str());
			
			bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), path.c_str(), true);

			if (!ret)
			{
				printf("Unable to load scene\n");
				return false;
			}
			
			std::unordered_map <size_t, size_t> materials_map;			
			
			Material defaultMat;
			int defaultMatID = scene->AddMaterial(defaultMat);
			
			size_t loadedMaterials = materials.size();

			// store used materials
			for (size_t s = 0; s < shapes.size(); s++) 
			{
				size_t mids_in_mesh = shapes[s].mesh.material_ids.size();
				for (size_t mids = 0; mids < mids_in_mesh; mids++) {
					materials_map[shapes[s].mesh.material_ids[mids]] = -1;
				}			
			}
			
			//convert materials to our disney brdf
			
			//assign materials to seshes and save meshes
			/*				
				for(auto it = materials_map.begin(); it != materials_map.end(); ++it) {
					// cout << it->first << " : " << it->second << endl;
					// конвертнуть и создать материал и поместить в значение materials_map
				}
				
				// ! учесть materials_map[id] чтоб был <= loadedMaterials, т.е. если меньше, то мат не загружен и надо присвоить default
				
				//разделить меши по материалам. один материал - один мэш
				if( mids_in_mesh < 2) {
					//1 назначить материал. если 0 - назначить default mat
				} else {
					//разделить мэш по материалам - создать несколько мэшей
				}
				*/

        return true;
	}

        

    bool LoadSceneFromFile(const std::string &filename, Scene *scene, RenderOptions& renderOptions)
    {				
		clock_t time1, time2;
		
		char *subString, *p;
		std::string format = "scene";
		std::string fn = "";
						
		subString = strrchr((char*) filename.c_str(), '.');
		p = subString;
		for ( ; *p; ++p) *p = tolower(*p);
		if(p - subString > 1) {
			format.assign(subString + 1);
		}
		
		time1 = clock();

		Log("Loading Scene..\n");
		
		if(format == "obj") {
			if(! LoadSceneFromObjFile(filename, scene, renderOptions)) return false;
		} else
		if(format == "scene") {
			const size_t buf_sz = 32 * 1024;
			char buf[buf_sz];
			
			FILE* file;
			file = fopen(filename.c_str(), "r");

			if (!file)
			{
				Log("Couldn't open %s for reading\n", filename.c_str());
				return false;
			}

			setvbuf(file, buf, _IOLBF, buf_sz);

			std::unordered_map<std::string, int> materialMap;
			std::vector<std::string> albedoTex;
			std::vector<std::string> metallicRoughnessTex;
			std::vector<std::string> normalTex;
			std::string path = filename.substr(0, filename.find_last_of("/\\")) + "/";

			char line[kMaxLineLength];

			//Defaults
			Material defaultMat;
			scene->AddMaterial(defaultMat);

			bool cameraAdded = false;

			int mat_id;
			
			float atDist1;

			while (fgets(line, kMaxLineLength, file))
			{
				// skip comments
				if (line[0] == '#')
					continue;

				// name used for materials and meshes
				char name[kMaxLineLength] = { 0 };

				//--------------------------------------------
				// Material

				if (sscanf(line, " material %s", name) == 1)
				{
					Material material;
					char albedoTexName[FILENAME_MAX] = "";
					char metallicRoughnessTexName[FILENAME_MAX] = "";
					char normalTexName[FILENAME_MAX] = "";

					while (fgets(line, kMaxLineLength, file))
					{
						// end group
						if (strchr(line, '}'))
							break;

						sscanf(line, " name %s", name);
						sscanf(line, " color %f %f %f", &material.albedo.x, &material.albedo.y, &material.albedo.z);
						sscanf(line, " emission %f %f %f", &material.emission.x, &material.emission.y, &material.emission.z);
						sscanf(line, " metallic %f", &material.metallic);
						sscanf(line, " roughness %f", &material.roughness);
						sscanf(line, " subsurface %f", &material.subsurface);
						sscanf(line, " specular %f", &material.specular);
						sscanf(line, " specularTint %f", &material.specularTint);
						sscanf(line, " anisotropic %f", &material.anisotropic);
						sscanf(line, " sheen %f", &material.sheen);
						sscanf(line, " sheenTint %f", &material.sheenTint);
						sscanf(line, " clearcoat %f", &material.clearcoat);
						sscanf(line, " clearcoatGloss %f", &material.clearcoatGloss);
						sscanf(line, " transmission %f", &material.transmission);
						sscanf(line, " ior %f", &material.ior);
						sscanf(line, " extinction %f %f %f", &material.extinction.x, &material.extinction.y, &material.extinction.z);
						sscanf(line, " atDistance %f", &material.atDistance);
						sscanf(line, " albedoTexture %s", albedoTexName);
						sscanf(line, " metallicRoughnessTexture %s", metallicRoughnessTexName);
						sscanf(line, " normalTexture %s", normalTexName);
					}

					
					// -log(state.mat.extinction) / state.mat.atDistance
					atDist1 = 1.0f / material.atDistance;
					material.extinction1.x = -log(material.extinction.x) * atDist1;
					material.extinction1.y = -log(material.extinction.y) * atDist1;
					material.extinction1.z = -log(material.extinction.z) * atDist1;

					// add material to map
					if (materialMap.find(name) == materialMap.end()) // New material
					{
						mat_id = scene->AddMaterial(material);
						materialMap[name] = mat_id;

						Material* pmat;

						pmat = &(scene->materials[mat_id]);
						
						// Albedo Texture
						if (strlen(albedoTexName) != 0) {
							fn = albedoTexName;
							if(!fileExists(fn)) fn = path + albedoTexName;
							scene->AddTexture(fn, &(pmat->albedoTexID));
							//printf("mat %d  albedoTexName=%s\n", mat_id, (path + albedoTexName).c_str());
						}
				 
						// MetallicRoughness Texture
						if (strlen(metallicRoughnessTexName) != 0) {
							fn = metallicRoughnessTexName;
							if(!fileExists(fn)) fn = path + metallicRoughnessTexName;
							scene->AddTexture(fn, &(pmat->metallicRoughnessTexID));
							//printf("mat %d  metallicRoughnessTexName=%s\n", mat_id, (path + metallicRoughnessTexName).c_str());
						}
		
						// Normal Map Texture
						if (strlen(normalTexName) != 0) {
							fn = normalTexName;
							if(!fileExists(fn)) fn = path + normalTexName;
							scene->AddTexture(fn, &(pmat->normalmapTexID));
							//printf("mat %d  normalTexName=%s\n", mat_id, (path + normalTexName).c_str());
						}
					}
				}

				//--------------------------------------------
				// Light

				if (strstr(line, "light"))
				{
					Light light;
					Vec3 v1, v2;
					char light_type[20] = "None";

					while (fgets(line, kMaxLineLength, file))
					{
						// end group
						if (strchr(line, '}'))
							break;

						sscanf(line, " position %f %f %f", &light.position.x, &light.position.y, &light.position.z);
						sscanf(line, " emission %f %f %f", &light.emission.x, &light.emission.y, &light.emission.z);

						sscanf(line, " radius %f", &light.radius);
						sscanf(line, " v1 %f %f %f", &v1.x, &v1.y, &v1.z);
						sscanf(line, " v2 %f %f %f", &v2.x, &v2.y, &v2.z);
						sscanf(line, " type %s", light_type);
					}

				   if (strcmp(light_type, "Quad") == 0)
					{
						light.type = LightType::RectLight;
						light.u = v1 - light.position;
						light.v = v2 - light.position;
						
						light.normal = Vec3::Cross(light.u, light.v);
						
						light.area = Vec3::Length(light.normal);
						
						light.normal = Vec3::Normalize(light.normal);

						light.uu = light.u * (1.0f / Vec3::Dot(light.u, light.u));
						light.vv = light.v * (1.0f / Vec3::Dot(light.v, light.v));

						light.radius = Vec3::Dot(light.normal, light.position);
					}
					else if (strcmp(light_type, "Sphere") == 0)
					{
						light.type = LightType::SphereLight;
						light.u.x = light.radius * light.radius;	//precalculated
						light.area = 4.0f * PI * light.u.x;
					}
					else if (strcmp(light_type, "Distant") == 0)
					{
						light.type = LightType::DistantLight;
						light.area = 0.0f;
						
						light.u = Vec3::Normalize(light.position);
					}


					scene->AddLight(light);
				}

				//--------------------------------------------
				// Camera

				if (strstr(line, "Camera"))
				{
					Vec3 position;
					Vec3 lookAt;
					float fov;
					float aperture = 0, focalDist = 1;

					while (fgets(line, kMaxLineLength, file))
					{
						// end group
						if (strchr(line, '}'))
							break;

						sscanf(line, " position %f %f %f", &position.x, &position.y, &position.z);
						sscanf(line, " lookAt %f %f %f", &lookAt.x, &lookAt.y, &lookAt.z);
						sscanf(line, " aperture %f ", &aperture);
						sscanf(line, " focaldist %f", &focalDist);
						sscanf(line, " fov %f", &fov);
					}

					delete scene->camera;
					scene->AddCamera(position, lookAt, fov);
					scene->camera->aperture = aperture;
					scene->camera->focalDist = focalDist;
					cameraAdded = true;
				}

				//--------------------------------------------
				// Renderer

				if (strstr(line, "Renderer"))
				{
					char envMap[FILENAME_MAX] = "";
					char enableRR[10] = "None";
					char *subString, *p;
					fn = "";

					while (fgets(line, kMaxLineLength, file))
					{
						// end group
						if (strchr(line, '}'))
							break;

						sscanf(line, " envMap %s", envMap);
						sscanf(line, " resolution %d %d", &renderOptions.resolution.x, &renderOptions.resolution.y);
						sscanf(line, " hdrMultiplier %f", &renderOptions.hdrMultiplier);
						sscanf(line, " hdrRotateX %f", &renderOptions.hdrRotate);
						sscanf(line, " hdrRotateY %f", &renderOptions.hdrRotateY);
						sscanf(line, " maxDepth %i", &renderOptions.maxDepth);
						sscanf(line, " tileWidth %i", &renderOptions.tileWidth);
						sscanf(line, " tileHeight %i", &renderOptions.tileHeight);
						sscanf(line, " enableRR %s", enableRR);
						sscanf(line, " RRDepth %i", &renderOptions.RRDepth);
						sscanf(line, " aaaPasses %i", &renderOptions.aaaPasses);
						sscanf(line, " aaaMinDiff %f", &renderOptions.aaa_minDist);
						sscanf(line, " aaaMaxDiff %f", &renderOptions.aaa_maxDist);
					}

					if (strlen(envMap) != 0)
					{
						subString = strrchr(envMap, '.');
						p = subString;
						for ( ; *p; ++p) *p = tolower(*p);
						
						if(subString != NULL) {
							fn = envMap;
							if(!fileExists(fn)) fn = path + envMap;
							
							if (strcmp(subString, ".hdr") == 0) {
								renderOptions.useEnvMap = scene->AddHDR(fn);
							}
							else if (strcmp(subString, ".exr") == 0) {								
								renderOptions.useEnvMap = scene->AddEXR(fn);
							}
						}
					}

					if (strcmp(enableRR, "False") == 0)
						renderOptions.enableRR = false;
					else if (strcmp(enableRR, "True") == 0)
						renderOptions.enableRR = true;
				}


				//--------------------------------------------
				// Mesh

				if (strstr(line, "mesh"))
				{
					fn = "";
					Mat4 xform;
					int material_id = 0; // Default Material ID
					char meshName[FILENAME_MAX] = "";
					
					float matrixTranslation[3], matrixRotation[3], matrixScale[3];

					matrixScale[0] = 1;
					matrixScale[1] = 1;
					matrixScale[2] = 1;

					matrixTranslation[0] = 0;
					matrixTranslation[1] = 0;
					matrixTranslation[2] = 0;

					matrixRotation[0] = 0;
					matrixRotation[1] = 0;
					matrixRotation[2] = 0;

					while (fgets(line, kMaxLineLength, file))
					{
						// end group
						if (strchr(line, '}'))
							break;

						char file[2048];
						char matName[100];

						sscanf(line, " name %[^\t\n]s", meshName);

						if (sscanf(line, " file %s", file) == 1)
						{
							fn = file;
							if(!fileExists(fn)) fn = path + file;
						}

						if (sscanf(line, " material %s", matName) == 1)
						{
							// look up material in dictionary
							if (materialMap.find(matName) != materialMap.end())
							{
								material_id = materialMap[matName];
							}
							else
							{
								Log("Could not find material %s\n", matName);
							}
						}

						
						sscanf(line, " position %f %f %f", &(matrixTranslation[0]), &(matrixTranslation[1]), &(matrixTranslation[2]));
						sscanf(line, " scale %f %f %f", &(matrixScale[0]), &(matrixScale[1]), &(matrixScale[2]));
						sscanf(line, " rotate %f %f %f", &(matrixRotation[0]), &(matrixRotation[1]), &(matrixRotation[2]));
					}
											
					
					if (!fn.empty())
					{
						int mesh_id = scene->AddMesh(fn);
						if (mesh_id != -1)
						{
							std::string instanceName;

							if (strlen(meshName) != 0)
							{
								instanceName = std::string(meshName);
							}
							else
							{
								std::size_t pos = fn.find_last_of("/\\");
								instanceName = fn.substr(pos + 1);
							}

							ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, (float*)&xform.data);
							
							MeshInstance instance1(instanceName, mesh_id, xform, material_id);
							scene->AddMeshInstance(instance1);
						}
					}
				}
			}

			fclose(file);

			if (!cameraAdded)
				scene->AddCamera(Vec3(0.0f, 0.0f, 10.0f), Vec3(0.0f, 0.0f, -10.0f), 35.0f);
			
			
			scene->renderOptions = renderOptions;
		}
		time2 = clock();
		printf("%.1fs\n", (float)(time2-time1)/(float)CLOCKS_PER_SEC);

        scene->CreateAccelerationStructures();

        return true;
    }
}