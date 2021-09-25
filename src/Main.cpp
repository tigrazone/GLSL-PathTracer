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
 * The above copyright noticeand this permission notice shall be included in all
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

#define _USE_MATH_DEFINES

#include <SDL2/SDL.h>
#include <GL/gl3w.h>

#include <time.h>
#include <math.h>

#include <cstring>
#include <string>

#include "Scene.h"
#include "TiledRenderer.h"
#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include "Loader.h"
#include "ajaxTestScene.h"

#include "boyTestScene.h"
#include "cornellTestScene.h"

#include "ImGuizmo.h"
#include "tinydir.h"

#include "Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "tinyexr.h"

#define PROGRAM_NAME "GLSL PathTracer"

using namespace std;
using namespace GLSLPT;

Scene* scene       = nullptr;
Renderer* renderer = nullptr;

std::vector<string> sceneFiles;

float mouseSensitivity = 0.01f;
bool keyPressed        = false;
int sampleSceneIndex   = 0;
int selectedInstance   = 0;
double lastTime        = SDL_GetTicks(); 
bool done = false;

std::string shadersDir = "../src/shaders/";
std::string assetsDir = "../assets/";

std::string sceneFile;


RenderOptions renderOptions;

int maxSPP = -1;
float maxRenderTime = -1.0f;

int saveEverySPP = -1;
int lastSavedSPP = -1;
float saveEveryTime = -1.0f;
float lastSaveTime = -1.0f;

std::string imgDefaultFilename = "img";
std::string imgDefaultFilenameExt = "png";

bool UIvisible = true;


bool addspp = false;

bool oldDefaultMaterial = false;

struct LoopData
{
    SDL_Window*   mWindow    = nullptr;
    SDL_GLContext mGLContext = nullptr;
};

void GetSceneFiles()
{
    tinydir_dir dir;
    int i;
    tinydir_open_sorted(&dir, assetsDir.c_str());

    for (i = 0; i < dir.n_files; i++)
    {
        tinydir_file file;
        tinydir_readfile_n(&dir, &file, i);

        if (std::string(file.extension) == "scene")
        {
            sceneFiles.push_back(assetsDir + std::string(file.name));
        }
    }

    tinydir_close(&dir);
}

bool LoadScene(std::string sceneName)
{
    delete scene;
    scene = new Scene();
	renderOptions.hdrRotate = 0.0f;
	renderOptions.hdrRotateY = 0.0f;
    if(!LoadSceneFromFile(sceneName, scene, renderOptions))
		return false;
    //loadCornellTestScene(scene, renderOptions);
    selectedInstance = 0;
    scene->renderOptions = renderOptions;
	return true;
}

bool InitRenderer()
{
    delete renderer;
    renderer = new TiledRenderer(scene, shadersDir);
    renderer->Init();
    return true;
}

// TODO: Fix occassional crashes when saving screenshot
void SaveFrame(const std::string filename, const std::string format="png")
{
    unsigned char* data = nullptr;
    int w, h;
    renderer->GetOutputBuffer(&data, w, h);
    stbi_flip_vertically_on_write(true);
	
	if(format == "png") {
		stbi_write_png(filename.c_str(), w, h, 3, data, w*3);
	} else
	if(format == "bmp") {
		stbi_write_bmp(filename.c_str(), w, h, 3, data);
	} else
	if(format == "tga") {
		stbi_write_tga(filename.c_str(), w, h, 3, data);
	} else	
	if(format == "jpg") {
		stbi_write_jpg(filename.c_str(), w, h, 3, data, 92);
	}
    delete data;
}


void SaveRawFrame(const std::string filename, const std::string format="exr")
{
	float* data = nullptr;
    int w, h;
	
	bool is_exr = (format == "exr");
	
    renderer->GetRawOutputBuffer(&data, w, h, is_exr);
	
	const char* err = NULL;
	int ret;
	
	if(is_exr) {
		ret = SaveEXR(data, w, h, 3 // =RGB
						,1 // = save as fp16 format, else fp32 bit
						,filename.c_str()
						,&err
					);
						
		  if (ret != TINYEXR_SUCCESS) {
			if (err) {
			  fprintf(stderr, "Save EXR err: %s(code %d)\n", err, ret);
			} else {
			  fprintf(stderr, "Failed to save EXR image. code = %d\n", ret);
			}
		}
	} else
	if(format == "hdr") {
		stbi_flip_vertically_on_write(true);
		ret = stbi_write_hdr(filename.c_str(), w, h, 3, data);
		if (ret == 0) {
		  fprintf(stderr, "Failed to save HDR image %s\n", filename.c_str());
		}
	}
	
    delete data;
}

void Render()
{
    auto io = ImGui::GetIO();
    renderer->Render();
    //const glm::ivec2 screenSize = renderer->GetScreenSize();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    renderer->Present();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void MoveCameraFromKeyboard(float multiply, float coef)
{	
			if(ImGui::IsKeyPressed(SDL_SCANCODE_W)) { // w
				scene->camera->SetRadius(-multiply);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_A)) { // a
				scene->camera->Strafe(-multiply, 0);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_S)) { // s
                scene->camera->SetRadius(multiply);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_D)) { // d
                scene->camera->Strafe(multiply, 0);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_R)) { // r
                scene->camera->Strafe(0, multiply);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_F)) { // f
                scene->camera->Strafe(0, -multiply);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_I)) { // i rot camera Y-
                scene->camera->OffsetOrientation(0, -coef);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_J)) { // j rot camera X-
                scene->camera->OffsetOrientation(-coef, 0);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_K)) { // k rot camera Y+
                scene->camera->OffsetOrientation(0, coef);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_L)) { // l rot camera X+
                scene->camera->OffsetOrientation(coef, 0);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_UP)) { 		// up arrow
				scene->camera->SetRadius(-multiply);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_RIGHT)) { 	// right arrow
				scene->camera->Strafe(-multiply, 0);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_DOWN)) { 	// down arrow
				scene->camera->SetRadius(multiply);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_LEFT)) { 	// left arrow
				scene->camera->Strafe(multiply, 0);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_PAGEUP)) { 	// PgUp
                scene->camera->Strafe(0, multiply);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_PAGEDOWN)) { // PgDn
                scene->camera->Strafe(0, -multiply);
                scene->camera->isMoving = true;
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_COMMA)) { // , fov -
				float fov = Math::Degrees(scene->camera->fov);
                if(fov - coef > 10.0f - coef*0.1f) {
					scene->camera->fov = Math::Radians(fov - coef);
					scene->camera->isMoving = true;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_PERIOD)) { // . fov +
				float fov = Math::Degrees(scene->camera->fov);
                if(fov + coef < 90.0f + coef*0.1f) {
					scene->camera->fov = Math::Radians(fov + coef);
					scene->camera->isMoving = true;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_X)) { // X env map Multiplier +
				if(renderOptions.hdrMultiplier + coef < MAXhdrMultiplier + coef*0.1f) {
					renderOptions.hdrMultiplier += coef;					
					scene->renderOptions = renderOptions;
					scene->camera->isMoving = true;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_Z)) { // Z env map Multiplier -
				if(renderOptions.hdrMultiplier - coef > 0.1f - coef*0.1f) {
					renderOptions.hdrMultiplier -= coef;					
					scene->renderOptions = renderOptions;
					scene->camera->isMoving = true;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_N)) { // N env map Rotate +
				if(renderOptions.hdrRotate + coef < 180.0f + coef*0.1f) {
					renderOptions.hdrRotate += coef;					
					scene->renderOptions = renderOptions;
					scene->camera->isMoving = true;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_M)) { // M env map Rotate -
				if(renderOptions.hdrRotate - coef > -180.0f - coef*0.1f) {
					renderOptions.hdrRotate -= coef;					
					scene->renderOptions = renderOptions;
					scene->camera->isMoving = true;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_Y)) { // Y env map RotateY +
				if(renderOptions.hdrRotateY + coef < 180.0f + coef*0.1f) {
					renderOptions.hdrRotateY += coef;					
					scene->renderOptions = renderOptions;
					scene->camera->isMoving = true;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_H)) { // H env map RotateY -
				if(renderOptions.hdrRotateY - coef > -180.0f - coef*0.1f) {
					renderOptions.hdrRotateY -= coef;					
					scene->renderOptions = renderOptions;
					scene->camera->isMoving = true;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_HOME)) { // Home movement step +
				multiply = coef * 0.01f;
				if(mouseSensitivity + multiply < 1.0f + multiply*0.1f) {
					mouseSensitivity += multiply;
				}
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_END)) { // End movement step -		
				multiply = coef * 0.01f;
				if(mouseSensitivity - multiply > 0.01f - multiply*0.1f) {
					mouseSensitivity -= multiply;
				}
			}
}


void Update(float secondsElapsed)
{
    keyPressed = false;

    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) && ImGui::IsAnyMouseDown() && !ImGuizmo::IsOver() )
    {
        if (ImGui::IsMouseDown(0))
        {
            ImVec2 mouseDelta = ImGui::GetMouseDragDelta(0, 0);
            scene->camera->OffsetOrientation(mouseDelta.x, mouseDelta.y);
            ImGui::ResetMouseDragDelta(0);
        }
        else if (ImGui::IsMouseDown(1))
        {
            ImVec2 mouseDelta = ImGui::GetMouseDragDelta(1, 0);
            scene->camera->SetRadius(mouseSensitivity * mouseDelta.y);
            ImGui::ResetMouseDragDelta(1);
        }
        else if (ImGui::IsMouseDown(2))
        {
            ImVec2 mouseDelta = ImGui::GetMouseDragDelta(2, 0);
            scene->camera->Strafe(mouseSensitivity * mouseDelta.x, mouseSensitivity * mouseDelta.y);
            ImGui::ResetMouseDragDelta(2);
        }
        scene->camera->isMoving = true;
    }
	

    ImGuiIO& io = ImGui::GetIO();
	
	int specKeys = 0;
	
	specKeys += io.KeyCtrl;
	specKeys += io.KeyShift; 
	specKeys += io.KeyAlt; 
	specKeys += io.KeySuper;
	
	float coef = 1.0f;

	if(specKeys < 2) {
		if(io.KeyCtrl) {
			coef = 0.1f;
			MoveCameraFromKeyboard(coef * mouseSensitivity, coef);
		} else			
		if(io.KeyShift) {
			coef = 10.0f;
			MoveCameraFromKeyboard(coef * mouseSensitivity, coef);
		} else			
		if(io.KeyAlt) {
			coef = 0.05f;
			MoveCameraFromKeyboard(coef * mouseSensitivity, coef);
		} else 
		if(specKeys == 0) {
			if(ImGui::IsKeyPressed(SDL_SCANCODE_P)) { // p print camera
				Vec3 pivot = scene->camera->getPivot();
				printf("CAMERA\nposition %.5f %.5f %.5f\n", scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
				printf("lookAt %.5f %.5f %.5f\n", pivot.x, pivot.y, pivot.z);
				printf("aperture %.5f\n", scene->camera->aperture * 1000.0f);
				printf("focaldist %.5f\n", scene->camera->focalDist);
				printf("fov %.5f\n\n", Math::Degrees(scene->camera->fov));
			} else
			if(ImGui::IsKeyPressed(SDL_SCANCODE_TAB)) { // TAB UI make visible/invisible
				UIvisible = ! UIvisible;
			} else
			MoveCameraFromKeyboard(coef * mouseSensitivity, coef);
		}
	} else
	if(specKeys == 2) {
		if(io.KeyCtrl && io.KeyShift) {
			coef = 100.0f;
			MoveCameraFromKeyboard(coef * mouseSensitivity, coef);
		}
	}

    renderer->Update(secondsElapsed);
}

void EditTransform(const float* view, const float* projection, float* matrix)
{
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
    {
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
    {
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
    {
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    }

    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation);
    ImGui::InputFloat3("Rt", matrixRotation);
    ImGui::InputFloat3("Sc", matrixScale);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
        if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        {
            mCurrentGizmoMode = ImGuizmo::LOCAL;
        }

        ImGui::SameLine();
        if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        {
            mCurrentGizmoMode = ImGuizmo::WORLD;
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(view, projection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, NULL);
}

void MainLoop(void* arg)
{
    LoopData &loopdata = *(LoopData*)arg;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
        {
            done = true;
        }
        if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                renderOptions.resolution = iVec2(event.window.data1, event.window.data2);
                scene->renderOptions = renderOptions;
                InitRenderer(); // FIXME: Not all textures have to be regenerated on resizing
            }

            if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(loopdata.mWindow))
            {
                done = true;
            }    
        }
    }
	
	
	int samplesNow = renderer->GetSampleCount();
	float renderTimeNow = renderer->GetRenderTime();
	
			//show samples and render time in window title
			char wtitle[1000];
			
			sprintf(wtitle, "%s | %d samples, time %.1fs", PROGRAM_NAME, samplesNow, renderTimeNow);
			SDL_SetWindowTitle(loopdata.mWindow, wtitle);
			

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(loopdata.mWindow);
    ImGui::NewFrame();
    ImGuizmo::SetOrthographic(true);
			

	if(UIvisible) {
    ImGuizmo::BeginFrame();
    {
        ImGui::Begin("Settings");

        ImGui::Text("Samples: %d ", samplesNow);
        ImGui::Text("Render time: %.1fs", renderTimeNow);
		
		if( maxSPP == samplesNow || (renderTimeNow > maxRenderTime && maxRenderTime>0.0f))
		{
			printf("%d samples. render time: %.1fs\n", samplesNow, renderTimeNow);
			
			std::string fn = imgDefaultFilename;
			fn += "_"+to_string(samplesNow)+"spp";
			fn += "_"+to_string((int)renderTimeNow)+"s";
			fn += "."+imgDefaultFilenameExt;
			
						if(imgDefaultFilenameExt == "hdr" || imgDefaultFilenameExt == "exr") {				
							SaveRawFrame(fn, imgDefaultFilenameExt);
						} else {	
							SaveFrame(fn, imgDefaultFilenameExt);
						}
            done = true;
		}
		
		if(saveEveryTime>0.0f && lastSaveTime<0.0f) {
			lastSaveTime = renderTimeNow;
		}
		
		if(saveEverySPP>0 && lastSavedSPP <0) {
			lastSavedSPP = 0;
		}
		
		if((saveEverySPP>0 && (samplesNow-lastSavedSPP) == saveEverySPP) || (renderTimeNow - lastSaveTime > saveEveryTime && saveEveryTime>0.0f))
		{
			printf("%d samples. render time: %.1fs\n", samplesNow, renderTimeNow);			
			
			std::string fn = imgDefaultFilename;
			if(addspp) {
				fn += "_"+to_string(samplesNow)+"spp";
				fn += "_"+to_string((int)renderTimeNow)+"s";
			}
			fn += "."+imgDefaultFilenameExt;
			
			lastSaveTime = renderTimeNow;
			lastSavedSPP = samplesNow;
			
						if(imgDefaultFilenameExt == "hdr" || imgDefaultFilenameExt == "exr") {				
							SaveRawFrame(fn, imgDefaultFilenameExt);
						} else {	
							SaveFrame(fn, imgDefaultFilenameExt);
						}
		}

        if (ImGui::Button("Save image"))
        {
            const std::string selected = saveFileDialog("Save image", assetsDir, { "*.png", "*.jpg", "*.tga", "*.bmp", "*.exr", "*.hdr" });
			if(!selected.empty()) {
						char *subString, *p;
						std::string format = "png";
						
						subString = strrchr((char*) selected.c_str(), '.');
						p = subString;
						for ( ; *p; ++p) *p = tolower(*p);
						if(p - subString > 1) {
							format.assign(subString + 1);
							// printf("format=%s\n", format.c_str());
						}
						
						if(format == "hdr" || format == "exr") {				
							SaveRawFrame(selected, format);
						} else {	
							SaveFrame(selected, format);
						}
			}
        }


        if (ImGui::Button("Load Scene"))
        {			
			const std::string selected = openFileDialog("Select a scene file", assetsDir, { "*.scene", "*.obj"
			// , "*.pbf", "*.pbrt"  
			});
			if(!selected.empty()) {
					if(LoadScene(selected)) {
						sceneFile = selected;
					} else {
						// reload previously loaded scene
						LoadScene(sceneFile);
					}
					
					SDL_RestoreWindow(loopdata.mWindow);
					SDL_SetWindowSize(loopdata.mWindow, renderOptions.resolution.x, renderOptions.resolution.y);
					InitRenderer();
				}
        }
		
		
        std::vector<const char*> scenes;
        for (int i = 0; i < sceneFiles.size(); ++i)
        {
            scenes.push_back(sceneFiles[i].c_str());
        }

        if (ImGui::Combo("Scene", &sampleSceneIndex, scenes.data(), scenes.size()))
        {
            if(LoadScene(sceneFiles[sampleSceneIndex])) {
				sceneFile = sceneFiles[sampleSceneIndex];
			}
            SDL_RestoreWindow(loopdata.mWindow);
            SDL_SetWindowSize(loopdata.mWindow, renderOptions.resolution.x, renderOptions.resolution.y);
            InitRenderer();
        }

        bool optionsChanged = false;

        ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 1.0f);

        if (ImGui::CollapsingHeader("Render Settings"))
        {
            bool requiresReload = false;
            Vec3* bgCol = &renderOptions.bgColor;

            optionsChanged |= ImGui::SliderInt("Max Depth", &renderOptions.maxDepth, 1, 10);
            requiresReload |= ImGui::Checkbox("Enable EnvMap", &renderOptions.useEnvMap);
			
                //show is loaded or no env map			
                ImGui::TextDisabled(scene->hdrData != nullptr ? "*" : " ");
                if (ImGui::IsItemHovered())
                {
					std::string fn = "";
					std::string LOADEDfn = "loaded ";
					if(scene->hdrData != nullptr) {
						fn = scene->HDRfn.substr(scene->HDRfn.find_last_of("/\\") + 1);
						LOADEDfn += fn;
					}
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(scene->hdrData != nullptr ? LOADEDfn.c_str() : "NOT loaded");
                    ImGui::EndTooltip();
                }
				// IsItemClicked()
				
				ImGui::SameLine();
				if (ImGui::Button("Load EnvMap"))
				{					
					const std::string envMap = openFileDialog("Select environment map file", assetsDir, { "*.hdr", "*.exr"});
					if(!envMap.empty()) {
						char *subString, *p;
						
						subString = strrchr((char*) envMap.c_str(), '.');
						p = subString;
						for ( ; *p; ++p) *p = tolower(*p);
						
						if(subString != NULL) {
							if (strcmp(subString, ".hdr") == 0) {
								renderOptions.useEnvMap = scene->AddHDR(envMap);
								scene->renderOptions = renderOptions;
								InitRenderer();
							}
							else if (strcmp(subString, ".exr") == 0) {
								renderOptions.useEnvMap = scene->AddEXR(envMap);
								scene->renderOptions = renderOptions;
								InitRenderer();
							}
						}
					}
				}
				
            optionsChanged |= ImGui::SliderFloat("HDR multiplier", &renderOptions.hdrMultiplier, 0.1f, MAXhdrMultiplier, "%.2f");
            optionsChanged |= ImGui::SliderFloat("Xrotate HDR ", &renderOptions.hdrRotate, -180.0f, 180.0f, "%.2f");
            optionsChanged |= ImGui::SliderFloat("Yrotate HDR", &renderOptions.hdrRotateY, -180.0f, 180.0f, "%.2f");
            requiresReload |= ImGui::Checkbox("Enable RR", &renderOptions.enableRR);
            requiresReload |= ImGui::SliderInt("RR Depth", &renderOptions.RRDepth, 1, 10);
            requiresReload |= ImGui::Checkbox("Enable Constant BG", &renderOptions.useConstantBg);
            optionsChanged |= ImGui::ColorEdit3("Background Color", (float*)bgCol, 0);
            ImGui::Checkbox("Enable Denoiser", &renderOptions.enableDenoiser);
            ImGui::SliderInt("Number of Frames to skip", &renderOptions.denoiserFrameCnt, 5, 50);
            
            if (requiresReload)
            {
                scene->renderOptions = renderOptions;
                InitRenderer();
            }

            scene->renderOptions.enableDenoiser = renderOptions.enableDenoiser;
            scene->renderOptions.denoiserFrameCnt = renderOptions.denoiserFrameCnt;
        }
        
        if (ImGui::CollapsingHeader("Camera"))
        {
            float fov = Math::Degrees(scene->camera->fov);
            float aperture = scene->camera->aperture * 1000.0f;
            optionsChanged |= ImGui::SliderFloat("Fov", &fov, 10, 90, "%.2f");
            scene->camera->SetFov(fov);
            optionsChanged |= ImGui::SliderFloat("Aperture", &aperture, 0.0f, 10.8f, "%.2f");
            scene->camera->aperture = aperture / 1000.0f;
            optionsChanged |= ImGui::SliderFloat("Focal Distance", &scene->camera->focalDist, 0.01f, 50.0f, "%.2f");
            // ImGui::Text("Pos: %.2f, %.2f, %.2f", scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
			
			if (ImGui::Button("Reset Camera"))
			{
                scene->camera->Reset();
				optionsChanged = true;
			}
        }

        scene->camera->isMoving = false;

        if (optionsChanged)
        {
            scene->renderOptions = renderOptions;
            scene->camera->isMoving = true;
        }

        if (ImGui::CollapsingHeader("Objects"))
        {
            bool objectPropChanged = false;

            std::vector<std::string> listboxItems;
            for (int i = 0; i < scene->meshInstances.size(); i++)
            {
                listboxItems.push_back(scene->meshInstances[i].name);
            }

            // Object Selection
            ImGui::ListBoxHeader("Instances");
            for (int i = 0; i < scene->meshInstances.size(); i++)
            {
                bool is_selected = selectedInstance == i;
                if (ImGui::Selectable(listboxItems[i].c_str(), is_selected))
                {
                    selectedInstance = i;
                }
            }
            ImGui::ListBoxFooter();

            ImGui::Separator();
            ImGui::Text("Materials");
			
			size_t nowMaterialId = scene->meshInstances[selectedInstance].materialID;
			Material *nowMat = &scene->materials[nowMaterialId];

            // Material Properties
            Vec3 *albedo   = &(*nowMat).albedo;
            Vec3 *emission = &(*nowMat).emission;
            Vec3 *extinction = &(*nowMat).extinction;
			
			Vec3 emissionColor = *emission;
            float emissionPower = Vec3::Length(emissionColor);
			if(emissionPower < 1.0f) {
				emissionPower = 1.0f;
			}
			
			if(emissionColor.x > 1.0f || emissionColor.y > 1.0f || emissionColor.z > 1.0f) { 
				emissionColor = Vec3::Normalize(*emission);
			}

            objectPropChanged |= ImGui::ColorEdit3("Albedo", (float*)albedo, 0);
            objectPropChanged |= ImGui::SliderFloat("Metallic",  &(*nowMat).metallic, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("Roughness", &(*nowMat).roughness, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("Specular", &(*nowMat).specular, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("SpecularTint", &(*nowMat).specularTint, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("Subsurface", &(*nowMat).subsurface, 0.0f, 1.0f, "%.2f");
            //objectPropChanged |= ImGui::SliderFloat("Anisotropic", &(*nowMat).anisotropic, 0.0f, 1.0f);
            objectPropChanged |= ImGui::SliderFloat("Sheen", &(*nowMat).sheen, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("SheenTint", &(*nowMat).sheenTint, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("Clearcoat", &(*nowMat).clearcoat, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("clearcoatGloss", &(*nowMat).clearcoatGloss, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("Transmission", &(*nowMat).transmission, 0.0f, 1.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("Ior", &(*nowMat).ior, 1.00f, 10.0f, "%.2f");
            objectPropChanged |= ImGui::SliderFloat("atDistance", &(*nowMat).atDistance, 0.05f, 10.0f, "%.2f");
            objectPropChanged |= ImGui::ColorEdit3("Extinction", (float*)extinction, 0);
            
			bool emissionChanged = false;
			
			emissionChanged |= ImGui::ColorEdit3("Emission", (float*)&emissionColor, 0);
			emissionChanged |= ImGui::SliderFloat("Emission power", &emissionPower, 1.0f, 1000.0f, "%.2f");
			
			// SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
			
			if (emissionChanged) 
			{
				*emission = emissionColor * emissionPower;
                objectPropChanged = true;
			}

            // Transforms Properties
            ImGui::Separator();
            ImGui::Text("Transforms");
            {
                float viewMatrix[16];
                float projMatrix[16];

                auto io = ImGui::GetIO();
                scene->camera->ComputeViewProjectionMatrix(viewMatrix, projMatrix, io.DisplaySize.x / io.DisplaySize.y);
                Mat4 xform = scene->meshInstances[selectedInstance].transform;

                EditTransform(viewMatrix, projMatrix, (float*)&xform);

                if (memcmp(&xform, &scene->meshInstances[selectedInstance].transform, sizeof(float) * 16))
                {
                    scene->meshInstances[selectedInstance].transform = xform;
                    objectPropChanged = true;
                }
            }

            if (objectPropChanged)
            {
                scene->RebuildInstances();
            }
        }
        ImGui::End();
      }
	}

    double presentTime = SDL_GetTicks();
    Update((float)(presentTime - lastTime));
    lastTime = presentTime;
    glClearColor(0., 0., 0., 0.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    Render();
    SDL_GL_SwapWindow(loopdata.mWindow);
}

int main(int argc, char** argv)
{
    srand((unsigned int)time(0));
	
    bool testAjax = false;
    bool testBoy = false;
	std::string arg;
	renderOptions.aaaPasses = 0;
	renderOptions.aaaBreaks = false;
	
    oldDefaultMaterial = true;

    for (int i = 1; i < argc; ++i)
    {
        arg.assign(argv[i]);
		
        if (arg == "-s" || arg == "--scene")
        {
            sceneFile = argv[++i];
        }
        else
        if (arg == "-aaa")
        {
            renderOptions.aaaPasses = atoi(argv[++i]);
        }
        else
        if (arg == "-aaaBREAKS")
        {
            renderOptions.aaaBreaks = true;
        }
        else
        if (arg == "-spp")
        {
            maxSPP = atoi(argv[++i]);
        }
        else 
        if (arg == "-time")
        {
            maxRenderTime = atof(argv[++i]);
        }
        else 
        if (arg == "-testAjax")
        {
            testAjax = true;
        }
        else
        if (arg == "-saveEverySpp")
        {
            saveEverySPP = atoi(argv[++i]);
        }
        else 
        if (arg == "-saveEveryTime")
        {
            saveEveryTime = atof(argv[++i]);
        }
        else 
        if (arg == "-addspp")
        {
            addspp = true;
        }
        else 
        if (arg == "-testBoy")
        {
            testBoy = true;
        }
        else
        if (arg == "-oldDefaultMaterial")
        {
            oldDefaultMaterial = true;
        }
        else
        if (arg == "-newDefaultMaterial")
        {
            oldDefaultMaterial = false;
        }
		else			
        if (arg == "-o")
        {
            imgDefaultFilename = argv[++i];
			
			char *subString1, *p1, *pp1, *fn;			
			size_t str_sz = imgDefaultFilename.length();
			
			pp1 = new char [ str_sz + 1];
			
			strcpy(pp1, imgDefaultFilename.c_str());
						
						subString1 = strrchr(pp1, '.');
						
						fn =  new char [ subString1 - pp1 ];
						strncpy(fn, pp1, subString1 - pp1);
						fn[ subString1 - pp1 ] = 0;						
						imgDefaultFilename.assign(fn);
						
						p1 = subString1;
						for ( ; *p1; ++p1) *p1 = tolower(*p1);
						
						if(p1 - subString1 > 1) {
							imgDefaultFilenameExt.assign(subString1 + 1);
						}
			
			delete[] pp1;
			delete[] fn;
        }
        else 
		if (arg[0] == '-')
        {
            printf("Unknown option %s \n'", arg.c_str());
            exit(0);
        }
    }

	if (testAjax) 
	{
			scene = new Scene();
			loadAjaxTestScene(scene, renderOptions);

			scene->renderOptions = renderOptions;
	} else 
	if (testBoy) 
	{
			scene = new Scene();
			loadBoyTestScene(scene, renderOptions);

			scene->renderOptions = renderOptions;
	} else {
		if (!sceneFile.empty())
		{
			scene = new Scene();

			if (!LoadSceneFromFile(sceneFile, scene, renderOptions))
				exit(0);

			scene->renderOptions = renderOptions;
			std::cout << "Scene Loaded\n\n";
		}
		else
		{
			GetSceneFiles();
			
            if(LoadScene(sceneFiles[sampleSceneIndex])) {
				sceneFile = sceneFiles[sampleSceneIndex];
			}
		}
	}

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    LoopData loopdata;

#ifdef __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    loopdata.mWindow = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, renderOptions.resolution.x, renderOptions.resolution.y, window_flags);

    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    loopdata.mGLContext = SDL_GL_CreateContext(loopdata.mWindow);
    if (!loopdata.mGLContext)
    {
        fprintf(stderr, "Failed to initialize GL context!\n");
        return 1;
    }
    SDL_GL_SetSwapInterval(0); // Disable vsync

    // Initialize OpenGL loader
#if GL_VERSION_3_2
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplSDL2_InitForOpenGL(loopdata.mWindow, loopdata.mGLContext);

    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGui::StyleColorsDark();
    if (!InitRenderer())
        return 1;

    while (!done)
    {
        MainLoop(&loopdata);
    }
	
	fflush(stdout);
        
    delete renderer;
    delete scene;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(loopdata.mGLContext);
    SDL_DestroyWindow(loopdata.mWindow);
    SDL_Quit();
    return 0;
}

