#include <SDL.h>
#include <SDL_image.h>
#include "glad/glad.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "spectrum.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <vector>

//#define RENDERING
const int NUMSAMPLES = 50000;
const int NUMSAMPLESPERFRAME = 10;
const int PATHLENGTH = 5;
const float EXPOSURE = 1.0f;

struct sphere {
	float pos[3];
	float radius;
	int materialID;
};

struct plane {
	float pos[3];
	int materialID;
};

struct box {
	float pos[3];
	float rotation[3];
	float size[3];
	int materialID;
};

struct lens {
	float pos[3];
	float rotation[3];
	float radius;
	float focalLength;
	bool isConverging;
	int materialID;
};

struct material {
	float reflection[3];
	float emission[2];
};

void world1(glm::vec3& cameraPos, glm::vec2& cameraAngle, std::vector<sphere>& spheres, std::vector<plane>& planes, std::vector<box>& boxes, std::vector<lens>& lenses, std::vector<material>& materials) {
	// Camera
	cameraPos = glm::vec3(6.332f, 3.855f, 3.140f);
	cameraAngle = glm::vec2(225.093f, -31.512f);

	// Spheres
	sphere sphere1 = { { 0.0f, 1.0f, 0.0f }, 1.0f, 1 };
	sphere sphere2 = { { 5.0f, 1.0f, -1.0f }, 1.0f, 2 };
	sphere sphere3 = { { 0.0f, 4.0f, -3.0f }, 1.0f, 3 };
	spheres.push_back(sphere1);
	spheres.push_back(sphere2);
	spheres.push_back(sphere3);

	// Planes
	plane plane1 = { { 0.0f, 0.0f, 0.0f }, 1 };
	planes.push_back(plane1);

	// Boxes
	box box1 = { { 3.0f, 0.75f, 1.0f }, { 0.0f, 58.31f, 0.0f }, { 1.5f, 1.5f, 1.5f }, 1 };
	boxes.push_back(box1);
	
	// Lenses
	lens lens1 = { { 5.0f, 1.2f, -4.0f }, { 0.0f, 0.0f, 0.0f }, 1.2f, 1.0f, true, 1 };
	lenses.push_back(lens1);

	// Materials
	material material1 = { { 550.0f, 100.0f, 0 }, { 5500.0f, 0.0f } };
	material material2 = { { 470.0f, 6.0f, 0 }, { 5500.0f, 0.0f } };
	material material3 = { { 550.0f, 0.0f, 0 }, { 5500.0f, 0.38f } };
	materials.push_back(material1);
	materials.push_back(material2);
	materials.push_back(material3);
}

void UpdateCameraPos(glm::vec3& cameraPos, glm::vec3 deltaCamPos, glm::vec2 cameraAngle) {
	// http://www.songho.ca/opengl/gl_anglestoaxes.html
	glm::vec2 theta = glm::vec2(-cameraAngle.y, cameraAngle.x);
	glm::mat3 mX = glm::mat3(1.0f, 0.0f, 0.0f, 0.0f, glm::cos(theta.x), -glm::sin(theta.x), 0.0f, glm::sin(theta.x), glm::cos(theta.x));
	glm::mat3 mY = glm::mat3(glm::cos(theta.y), 0.0f, glm::sin(theta.y), 0.0f, 1.0f, 0.0f, -glm::sin(theta.y), 0.0f, glm::cos(theta.y));
	glm::mat3 m = mX * mY;
	cameraPos = cameraPos + deltaCamPos * m;
}

float SpectralPowerDistribution(float l, float l_peak, float d, float invert) {
	// Spectral Power Distribution Function Calculated On The Basis Of Peak Wavelength And Standard Deviation
	// Using Gaussian Function To Predict Spectral Radiance
	// In Reality, Spectral Radiance Function Has Different Shapes For Different Objects Also Looks Much Different Than This
	float x = (l - l_peak) / (2.0f * d * d);
	float radiance = exp(-x * x);
	radiance = glm::mix(radiance, 1.0f - radiance, invert);
	return radiance;
}

float BlackBodyRadiation(float l, float T) {
	// Plank's Law
	return (1.1910429724e-16f * pow(l, -5.0f)) / (exp(0.014387768775f / (l * T)) - 1.0f);
}

float BlackBodyRadiationPeak(float T) {
	// Derived By Substituting Wien's Displacement Law On Plank's Law
	return 4.0956746759e-6f * pow(T, 5.0f);
}

void ItemsTable(const char* name, int& selection, int id, int size) {
	for (int i = id; i < (size + id); i++) {
		ImGui::PushID(i);
		ImGui::TableNextRow();
		char label[32];
		sprintf_s(label, name, i - id + 1);
		ImGui::TableSetColumnIndex(0);
		bool isSelected = selection == i;
		if (ImGui::Selectable(label, &isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
			if (isSelected) {
				selection = i;
			}
		}
		ImGui::PopID();
	}
}

void SDLHandleEvents(bool& isRunning, glm::ivec2 resolution, glm::vec2& cursorPos, glm::vec2& cameraAngle, glm::vec3& deltaCamPos, bool& isReset, bool isWindowFocused) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT) {
			isRunning = false;
		}
	}

	glm::vec2 cursorPos1 = glm::vec2((float)(ImGui::GetMousePos().x) / (float)resolution.x, (float)(resolution.y - ImGui::GetMousePos().y) / (float)resolution.y);
	if (!isWindowFocused) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			isReset = true;
			glm::vec2 dxdy = cursorPos1 - cursorPos;
			cameraAngle = cameraAngle - (360.0f * dxdy);
			if (cameraAngle.x > 360.0f) {
				cameraAngle.x = cameraAngle.x - 360.0f;
			}
			if (cameraAngle.x < 0.0f) {
				cameraAngle.x = 360.0f + cameraAngle.x;
			}
			if (cameraAngle.y > 90.0f) {
				cameraAngle.y = 90.0f;
			}
			if (cameraAngle.y < -90.0f) {
				cameraAngle.y = -90.0f;
			}
		}
	}
	cursorPos = cursorPos1;
	deltaCamPos.x = ((float)(ImGui::IsKeyDown(ImGuiKey_D)) - (float)(ImGui::IsKeyDown(ImGuiKey_A)));
	deltaCamPos.y = ((float)(ImGui::IsKeyDown(ImGuiKey_E)) - (float)(ImGui::IsKeyDown(ImGuiKey_Q)));
	deltaCamPos.z = ((float)(ImGui::IsKeyDown(ImGuiKey_W)) - (float)(ImGui::IsKeyDown(ImGuiKey_S)));
	if ((deltaCamPos.x != 0.0f) || (deltaCamPos.y != 0.0f) || (deltaCamPos.z != 0.0f)) {
		isReset = true;
	}
}

/* Thanks For Helping Me Here :)
https://stackoverflow.com/a/65817254 */
void flipSurface(SDL_Surface* surface)
{
	SDL_LockSurface(surface);

	int pitch = surface->pitch; // row size
	char* temp = new char[pitch]; // intermediate buffer
	char* pixels = (char*)surface->pixels;

	for (int i = 0; i < surface->h / 2; ++i) {
		// get pointers to the two rows to swap
		char* row1 = pixels + i * pitch;
		char* row2 = pixels + (surface->h - i - 1) * pitch;

		// swap rows
		memcpy(temp, row1, pitch);
		memcpy(row1, row2, pitch);
		memcpy(row2, temp, pitch);
	}

	delete[] temp;

	SDL_UnlockSurface(surface);
}

void AttachShader(GLuint program, GLenum type, const char* code) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &code, NULL);
	glCompileShader(shader);
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		char msg[1024];
		glGetShaderInfoLog(shader, 1024, NULL, msg);
		std::cout << "Shader Compile Error: " << std::endl << msg << std::endl;
	}
	glAttachShader(program, shader);
	glDeleteShader(shader);
}

const std::string ReadFile(std::string FileName) {
	std::ostringstream sstream;
	std::ifstream File(FileName);
	sstream << File.rdbuf();
	const std::string FileData(sstream.str());
	return FileData;
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	const char* glsl_version = "#version 460";
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	int width = 1280;
	int height = 720;
	Uint32 WindowFlags;
	#ifdef RENDERING
		WindowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;
	#else
		WindowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
	#endif
	SDL_Window* window = SDL_CreateWindow("Path Tracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, WindowFlags);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(0);  // VSYNC

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	gladLoadGLLoader(SDL_GL_GetProcAddress);
	std::cout << "Vendor:   " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version:  " << glGetString(GL_VERSION) << std::endl;
	
	GLuint shaderProgram = glCreateProgram();
	glObjectLabel(GL_PROGRAM, shaderProgram, -1, "ShaderProgram");
	GLuint frameBufferProgram = glCreateProgram();
	glObjectLabel(GL_PROGRAM, frameBufferProgram, -1, "FrameBufferProgram");

	std::string currentDirectory = std::filesystem::current_path().string();
	std::replace(currentDirectory.begin(), currentDirectory.end(), '\\', '/');
	currentDirectory = currentDirectory.append("/");
	std::string vertexShaderDirectory = currentDirectory;
	std::string pixelShaderDirectory = currentDirectory;
	std::string pixelFrameBufferDirectory = currentDirectory;
	vertexShaderDirectory.append("VertexShader.glsl");
	pixelShaderDirectory.append("PixelShader.glsl");
	pixelFrameBufferDirectory.append("PixelFrameBuffer.glsl");

	AttachShader(shaderProgram, GL_VERTEX_SHADER, ReadFile(vertexShaderDirectory).c_str());
	AttachShader(shaderProgram, GL_FRAGMENT_SHADER, ReadFile(pixelShaderDirectory).c_str());
	AttachShader(frameBufferProgram, GL_VERTEX_SHADER, ReadFile(vertexShaderDirectory).c_str());
	AttachShader(frameBufferProgram, GL_FRAGMENT_SHADER, ReadFile(pixelFrameBufferDirectory).c_str());
	
	float rectVertices[] = {
		// Coords    // texCoords
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,

		 1.0f,  1.0f,  1.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f
	};

	GLuint rectVAO, rectVBO;
	glGenVertexArrays(1, &rectVAO);
	glGenBuffers(1, &rectVBO);
	glBindVertexArray(rectVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), &rectVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

	GLuint FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	GLuint frameBufferTexture;
	glGenTextures(1, &frameBufferTexture);
	glBindTexture(GL_TEXTURE_2D, frameBufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBufferTexture, 0);

	GLuint RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);

	GLenum FBOStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (FBOStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "FrameBuffer Error: " << std::endl << FBOStatus << std::endl;
	}

	glLinkProgram(shaderProgram);
	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (success != GL_TRUE) {
		char msg[1024];
		glGetProgramInfoLog(shaderProgram, 1024, NULL, msg);
		std::cout << "Shader Link Error: " << std::endl << msg << std::endl;
		return 0;
	}

	glLinkProgram(frameBufferProgram);
	glGetProgramiv(frameBufferProgram, GL_LINK_STATUS, &success);
	if (success != GL_TRUE) {
		char msg[1024];
		glGetProgramInfoLog(frameBufferProgram, 1024, NULL, msg);
		std::cout << "FrameBuffer Link Error: " << std::endl << msg << std::endl;
		return 0;
	}

	glViewport(0, 0, width, height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);
	glUseProgram(frameBufferProgram);
	glUniform1i(glGetUniformLocation(frameBufferProgram, "screenTexture"), 0);

	bool isRunning = true;
	float FPS = 60.0f;
	float persistance = 0.0625f;
	float exposure = 1.0f;
	float lensDiameter = 1.65f;
	float lensFocalLength = 1.0f;
	float lensDistance = 0.25f;
	glm::vec2 cameraAngle = glm::vec2(0.0f, 0.0f);
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
	int samplesPerFrame = 1;
	int pathLength = 5;
#ifdef RENDERING
	exposure = EXPOSURE;
	samplesPerFrame = NUMSAMPLESPERFRAME;
	pathLength = PATHLENGTH;
	uint64_t start = SDL_GetTicks64();
#endif
	int frame = samplesPerFrame;
	int samples = samplesPerFrame;
	int prevSamples = samplesPerFrame;
	bool isBeginning = true;
#ifndef RENDERING
	glm::vec2 cursorPos = glm::vec2(0.0f, 0.0f);
	glm::vec3 deltaCamPos = glm::vec3(0.0f, 0.0f, 0.0f);
	bool isWindowFocused = false;
	std::vector <float> frames;
	std::vector <float> spectra;
#endif
	std::vector <sphere> spheres;
	sphere newsphere = { { 0.0f, 0.0f, 0.0f }, 1.0f, 1 };
	std::vector <plane> planes;
	plane newplane = { { 0.0f, 0.0f, 0.0f }, 1 };
	std::vector <box> boxes;
	box newbox = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 1 };
	std::vector <lens> lenses;
	lens newlens { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 1.0f, 1.0f, true, 1 };
	std::vector <material> materials;
	material newmaterial = { { 550.0f, 100.0f, 0 }, { 5500.0f, 0.0f } };
	world1(cameraPos, cameraAngle, spheres, planes, boxes, lenses, materials);

	while (isRunning) {
		bool isReset = false;
		bool isWinSizeChanged = false;

#ifndef RENDERING
		SDLHandleEvents(isRunning, glm::ivec2(width, height), cursorPos, cameraAngle, deltaCamPos, isReset, isWindowFocused);
		deltaCamPos *= 3.0f / FPS;
		UpdateCameraPos(cameraPos, deltaCamPos, glm::radians(cameraAngle));
		glm::ivec2 tempWindowSize{};
		SDL_GetWindowSize(window, &tempWindowSize.x, &tempWindowSize.y);
		if ((tempWindowSize.x != width) || (tempWindowSize.y != height)) {
			width = tempWindowSize.x;
			height = tempWindowSize.y;
			isWinSizeChanged = true;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		ImGuiWindowFlags WinFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize;

		{
			ImGui::Begin("Scene", NULL, WinFlags);
			ImGui::SetWindowPos(ImVec2(width - ImGui::GetWindowWidth(), 0));
			ImGui::Text("Render Time: %0.3f ms (%0.1f FPS)", 1000.0f / FPS, FPS);
			ImGui::PlotLines("", frames.data(), (int)frames.size(), 0, NULL, 0.0f, 30.0f, ImVec2(303, 100));
			ImGui::Text("Samples: %i", prevSamples);
			ImGui::Text("Camera Angle: (%0.3f, %0.3f)", cameraAngle.x, cameraAngle.y);
			ImGui::Text("Camera Pos: (%0.3f, %0.3f, %0.3f)", cameraPos.x, cameraPos.y, cameraPos.z);
			isReset |= ImGui::DragInt("Samples/Frame", &samplesPerFrame, 0.02f, 0, 100);
			isReset |= ImGui::DragInt("Path Length", &pathLength, 0.02f);
			ImGui::Separator();

			if (ImGui::CollapsingHeader("Camera")) {
				isReset |= ImGui::DragFloat("Persistance", &persistance, 0.00025f, 0.00025f, 1.0f, "%0.5f");
				isReset |= ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.01f, 20.0f, "%0.2f");
				isReset |= ImGui::DragFloat("Aperture Size", &lensDiameter, 0.01f, 0.01f, 100.0f, "%0.2f");
				isReset |= ImGui::DragFloat("Focal Length", &lensFocalLength, 0.01f, 0.5f, 100.0f, "%0.2f");
				isReset |= ImGui::DragFloat("Lens Distance", &lensDistance, 0.01f, 0.01f, 100.0f, "%0.2f");
			}
			ImGui::Separator();

			if (ImGui::CollapsingHeader("Objects")) {
				static int objectsSelection = 0;

				if (ImGui::Button("Add New Sphere")) {
					spheres.push_back(newsphere);
				}
				if (ImGui::Button("Add New Plane")) {
					planes.push_back(newplane);
				}
				if (ImGui::Button("Add New Box")) {
					boxes.push_back(newbox);
				}
				if (ImGui::Button("Add New Lens")) {
					lenses.push_back(newlens);
				}
				ImGui::Separator();

				int numObjs = 0;
				if ((objectsSelection < (numObjs + spheres.size())) && (objectsSelection > (numObjs - 1))) {
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Sphere %i", objectsSelection - numObjs + 1);
					isReset |= ImGui::DragFloat3("Position", spheres[objectsSelection - numObjs].pos, 0.01f);
					isReset |= ImGui::DragFloat("Radius", &spheres[objectsSelection - numObjs].radius, 0.01f);
					isReset |= ImGui::DragInt("Material ID", &spheres[objectsSelection - numObjs].materialID, 0.02f);
				}
				numObjs += (int)spheres.size();
				if ((objectsSelection < (numObjs + planes.size())) && (objectsSelection > (numObjs - 1))) {
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Plane %i", objectsSelection - numObjs + 1);
					isReset |= ImGui::DragFloat3("Position", planes[objectsSelection - numObjs].pos, 0.01f);
					isReset |= ImGui::DragInt("Material ID", &planes[objectsSelection - numObjs].materialID, 0.02f);
				}
				numObjs += (int)planes.size();
				if ((objectsSelection < (numObjs + boxes.size())) && (objectsSelection > (numObjs - 1))) {
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Box %i", objectsSelection - numObjs + 1);
					isReset |= ImGui::DragFloat3("Position", boxes[objectsSelection - numObjs].pos, 0.01f);
					isReset |= ImGui::DragFloat3("Rotation", boxes[objectsSelection - numObjs].rotation, 0.1f);
					isReset |= ImGui::DragFloat3("Size", boxes[objectsSelection - numObjs].size, 0.01f);
					isReset |= ImGui::DragInt("Material ID", &boxes[objectsSelection - numObjs].materialID, 0.02f);
				}
				numObjs += (int)boxes.size();
				if ((objectsSelection < (numObjs + lenses.size())) && (objectsSelection > (numObjs - 1))) {
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Lens %i", objectsSelection - numObjs + 1);
					isReset |= ImGui::DragFloat3("Position", lenses[objectsSelection - numObjs].pos, 0.01f);
					isReset |= ImGui::DragFloat3("Rotation", lenses[objectsSelection - numObjs].rotation, 0.1f);
					isReset |= ImGui::DragFloat("Radius", &lenses[objectsSelection - numObjs].radius, 0.01f);
					isReset |= ImGui::DragFloat("Focal Length", &lenses[objectsSelection - numObjs].focalLength, 0.01f);
					isReset |= ImGui::Checkbox("Convex Lens", &lenses[objectsSelection - numObjs].isConverging);
					isReset |= ImGui::DragInt("Material ID", &lenses[objectsSelection - numObjs].materialID, 0.02f);
				}
				numObjs += (int)lenses.size();
				ImGui::Separator();

				if (ImGui::BeginTable("Objects Table", 1)) {
					ImGui::TableSetupColumn("Object");
					ImGui::TableHeadersRow();
					ItemsTable("Sphere %i", objectsSelection, 0, (int)spheres.size());
					ItemsTable("Plane %i", objectsSelection, (int)(spheres.size()), (int)planes.size());
					ItemsTable("Box %i", objectsSelection, (int)(spheres.size() + planes.size()), (int)boxes.size());
					ItemsTable("Lens %i", objectsSelection, (int)(spheres.size() + planes.size() + boxes.size()), (int)lenses.size());
					ImGui::EndTable();
				}
			}
			ImGui::Separator();

			if (ImGui::CollapsingHeader("Materials")) {
				static int materialsSelection = 0;

				if (ImGui::Button("Add New Material")) {
					materials.push_back(newmaterial);
					isReset = true;
				}
				ImGui::Separator();

				ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Material %i", materialsSelection + 1);
				bool tmpIsInvert;
				ImGui::Text("Reflection");
				ImGui::PushID("Reflection");
				spectra.clear();
				for (int i = 1; i < 101; i++) {
					float x = 0.01f * float(i) * 330.0f + 390.0f;
					spectra.push_back(SpectralPowerDistribution(x, materials[materialsSelection].reflection[0], materials[materialsSelection].reflection[1], materials[materialsSelection].reflection[2]));
				}
				ImGui::PlotLines("", spectra.data(), (int)spectra.size(), 0, NULL, 0.0f, 1.0f, ImVec2(303, 100));
				isReset |= ImGui::DragFloat("Peak Lambda", &materials[materialsSelection].reflection[0], 1.0f, 0.0f, 1200.0f);
				isReset |= ImGui::DragFloat("Sigma", &materials[materialsSelection].reflection[1], 0.5f, 0.0f, 100.0f);
				tmpIsInvert = (bool)materials[materialsSelection].reflection[2];
				isReset |= ImGui::Checkbox("Invert", &tmpIsInvert);
				materials[materialsSelection].reflection[2] = (float)tmpIsInvert;
				ImGui::PopID();
				ImGui::Text("Emission");
				ImGui::PushID("Emission");
				spectra.clear();
				for (int i = 1; i < 101; i++) {
					float x = 0.01f * float(i) * 1200.0f * 1e-9f;
					spectra.push_back(BlackBodyRadiation(x, materials[materialsSelection].emission[0]) / BlackBodyRadiationPeak(materials[materialsSelection].emission[0]));
				}
				ImGui::PlotLines("", spectra.data(), (int)spectra.size(), 0, NULL, 0.0f, 1.0f, ImVec2(303, 100));
				isReset |= ImGui::DragFloat("Temperature", &materials[materialsSelection].emission[0], 5.0f);
				isReset |= ImGui::DragFloat("Luminosity", &materials[materialsSelection].emission[1], 0.1f);
				ImGui::PopID();
				ImGui::Separator();

				if (ImGui::BeginTable("Materials Table", 1)) {
					ImGui::TableSetupColumn("Materials");
					ImGui::TableHeadersRow();
					ItemsTable("Material %i", materialsSelection, 0, (int)materials.size());
					ImGui::EndTable();
				}
			}
				
			isWindowFocused = ImGui::IsWindowFocused();
			ImGui::End();
		}

		ImGui::Render();

#endif

		if (isWinSizeChanged) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

#ifndef RENDERING
		if (isWinSizeChanged) {
			glViewport(0, 0, width, height);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			frame = samplesPerFrame;
		}
		if (isReset || isWinSizeChanged) {
			samples = samplesPerFrame;
		}
#endif

		glUseProgram(shaderProgram);
		glUniform1i(glGetUniformLocation(shaderProgram, "frame"), frame);
		glUniform1i(glGetUniformLocation(shaderProgram, "samples"), samples);
		glUniform1i(glGetUniformLocation(shaderProgram, "prevSamples"), prevSamples);
		glUniform1f(glGetUniformLocation(shaderProgram, "FPS"), FPS);
		if (isWinSizeChanged || isBeginning) {
			glUniform2i(glGetUniformLocation(shaderProgram, "resolution"), width, height);
		}
		if (isReset || isBeginning) {
			glUniform2f(glGetUniformLocation(shaderProgram, "cameraAngle"), -cameraAngle.y, cameraAngle.x);
			glUniform3f(glGetUniformLocation(shaderProgram, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
			glUniform1f(glGetUniformLocation(shaderProgram, "persistance"), persistance);
			glUniform1f(glGetUniformLocation(shaderProgram, "exposure"), exposure);
			glUniform3f(glGetUniformLocation(shaderProgram, "lensData"), lensDiameter / 2.0f, lensFocalLength, lensDistance);
			glUniform1i(glGetUniformLocation(shaderProgram, "samplesPerFrame"), samplesPerFrame);
			glUniform1i(glGetUniformLocation(shaderProgram, "pathLength"), pathLength);
			glUniform1fv(glGetUniformLocation(shaderProgram, "CIEXYZ2006"), sizeof(CIEXYZ2006) / sizeof(float), CIEXYZ2006);
			int numObjects[] = { (int)spheres.size(), (int)planes.size(), (int)boxes.size(), (int)lenses.size() };
			glUniform1iv(glGetUniformLocation(shaderProgram, "numObjects"), sizeof(numObjects) / sizeof(int), numObjects);
			std::vector <float> objectsArray;
			for (int i = 0; i < spheres.size(); i++) {
				objectsArray.push_back(spheres[i].pos[0]);
				objectsArray.push_back(spheres[i].pos[1]);
				objectsArray.push_back(spheres[i].pos[2]);
				objectsArray.push_back(spheres[i].radius);
				objectsArray.push_back((float)spheres[i].materialID);
			}
			for (int i = 0; i < planes.size(); i++) {
				objectsArray.push_back(planes[i].pos[0]);
				objectsArray.push_back(planes[i].pos[1]);
				objectsArray.push_back(planes[i].pos[2]);
				objectsArray.push_back((float)planes[i].materialID);
			}
			for (int i = 0; i < boxes.size(); i++) {
				objectsArray.push_back(boxes[i].pos[0]);
				objectsArray.push_back(boxes[i].pos[1]);
				objectsArray.push_back(boxes[i].pos[2]);
				objectsArray.push_back(boxes[i].rotation[0]);
				objectsArray.push_back(boxes[i].rotation[1]);
				objectsArray.push_back(boxes[i].rotation[2]);
				objectsArray.push_back(boxes[i].size[0]);
				objectsArray.push_back(boxes[i].size[1]);
				objectsArray.push_back(boxes[i].size[2]);
				objectsArray.push_back((float)boxes[i].materialID);
			}
			for (int i = 0; i < lenses.size(); i++) {
				objectsArray.push_back(lenses[i].pos[0]);
				objectsArray.push_back(lenses[i].pos[1]);
				objectsArray.push_back(lenses[i].pos[2]);
				objectsArray.push_back(lenses[i].rotation[0]);
				objectsArray.push_back(lenses[i].rotation[1]);
				objectsArray.push_back(lenses[i].rotation[2]);
				objectsArray.push_back(lenses[i].radius);
				objectsArray.push_back(lenses[i].focalLength);
				objectsArray.push_back((float)lenses[i].isConverging);
				objectsArray.push_back((float)lenses[i].materialID);
			}
			glUniform1fv(glGetUniformLocation(shaderProgram, "objects"), (GLsizei)objectsArray.size(), objectsArray.data());
			std::vector <float> materialsArray;
			for (int i = 0; i < materials.size(); i++) {
				materialsArray.push_back(materials[i].reflection[0]);
				materialsArray.push_back(materials[i].reflection[1]);
				materialsArray.push_back(materials[i].reflection[2]);
				materialsArray.push_back(materials[i].emission[0]);
				materialsArray.push_back(materials[i].emission[1]);
			}
			glUniform1fv(glGetUniformLocation(shaderProgram, "materials"), (GLsizei)materialsArray.size(), materialsArray.data());
		}
		glBindVertexArray(rectVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(frameBufferProgram);
		glUniform1i(glGetUniformLocation(frameBufferProgram, "frame"), frame);
		glUniform1i(glGetUniformLocation(frameBufferProgram, "samples"), samples);
		if (isWinSizeChanged || isBeginning) {
			glUniform2i(glGetUniformLocation(frameBufferProgram, "resolution"), width, height);
		}
		glBindVertexArray(rectVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

#ifndef RENDERING
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		FPS = 1.0f / io.DeltaTime;
		if (frames.size() > 100) {
			for (size_t i = 1; i < frames.size(); i++) {
				frames[i - 1] = frames[i];
			}
			frames[frames.size() - 1] = 1000.0f / FPS;
		}
		else {
			frames.push_back(1000.0f / FPS);
		}
#endif

		SDL_GL_SwapWindow(window);

#ifdef RENDERING
		uint64_t end = SDL_GetTicks64();
		double timeElapsed = (double)(end - start) / 1000.0;
		double percentageDone = (double)frame / (double)NUMSAMPLES;
		double timeRemaining = (1.0 - percentageDone) * timeElapsed / percentageDone;
		std::cout << "Samples: " << frame << " / " << NUMSAMPLES << "\t" << "Time Elapsed: " << timeElapsed << "s" << "\t" << "Time Remaining: " << timeRemaining << "s" << " \r";
		bool isRenderingDone = (frame + 1) > NUMSAMPLES;
		if (isRenderingDone) {
			std::cout << std::endl << "Rendering Completed In " << timeElapsed << "s." << std::endl;
				
			unsigned char* framePixels = new unsigned char[width * height * 4];
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, framePixels);
			SDL_Surface* frameSurface = SDL_CreateRGBSurfaceFrom(framePixels, width, height, 8 * 4, width * 4, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
			flipSurface(frameSurface);
			IMG_SavePNG(frameSurface, "render.png");
			std::cout << "Rendered Image Has Been Successfully Saved." << std::endl;
			SDL_FreeSurface(frameSurface);
			delete[] framePixels;

			break;
		}
#endif

		prevSamples = samples;
		frame += samplesPerFrame;
		samples += samplesPerFrame;
		isBeginning = false;
	}
	
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	
	glDeleteTextures(1, &frameBufferTexture);
	glDeleteBuffers(1, &rectVBO);
	glDeleteVertexArrays(1, &rectVAO);
	glDeleteFramebuffers(1, &FBO);
	glDeleteProgram(frameBufferProgram);
	glDeleteProgram(shaderProgram);
	
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}