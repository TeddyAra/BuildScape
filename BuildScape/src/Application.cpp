#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <vector>
#include <stdint.h>
#include <cstdint>
#include <chrono>

#include "Input.h"
#include "Camera.h"
#include "Chunk.h"
#include "World.h"
#include "Renderer.h"
#include "Debug.h"
#include "Texture.h"

// unsigned 32 bit int, 26/32
// 
// x position   = 4 bits
// y position   = 4 bits
// z position   = 4 bits
// id			= 8 bits
// face up		= 1 bit
// face down	= 1 bit
// face right	= 1 bit
// face left	= 1 bit
// face front	= 1 bit
// face back	= 1 bit
//
// 1 bit  = 0x01 = 0 -   1
// 2 bits = 0x03 = 0 -   3
// 3 bits = 0x07 = 0 -   7
// 4 bits = 0x0F = 0 -  15
// 5 bits = 0x1F = 0 -  31
// 6 bits = 0x3F = 0 -  63
// 7 bits = 0x7F = 0 - 127
// 8 bits = 0xFF = 0 - 255

bool internalFaceCulling = true;
bool backFaceCulling = true;

// Game variables
std::string windowName = "BuildScape";
std::string gameVersion = "Alpha";

int windowWidth = 960;
int windowHeight = 540;
float voxelSize = 0.5f;
glm::vec3 skyCol = glm::vec3(0.5f, 0.8f, 0.9f);

glm::vec3 normalPos = glm::vec3(-2.0f, 8.0f, -2.0f);
glm::vec3 normalFront = glm::normalize(glm::vec3(1.0f, -0.5f, 1.0f));
glm::vec3 normalUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Classes
Camera camera(normalPos, normalFront, normalUp, 10.0f, 45.0f, 1.0f);
Renderer renderer;
World world(voxelSize, 4, &camera, &renderer);
Debug debug("Debug window", 300, windowHeight);

// Other variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool firstMouse = true;

int wireframe = 0;
bool recording = false;
float recordingTime = 10;
float recordingTimer = 0;
float recordCamSpeed = 3;
int frameCounter = 0;
float fpsCounter = 0;
bool uiCollapsed = false;

bool checkCurrentChunk = false;

// Input
void processInput(GLFWwindow* window) {
	checkCurrentChunk = false;

	if (Input::getKeyDown(GLFW_KEY_ESCAPE)) {
		Input::toggleIgnoreMouse();
	}

	// Reset
	if (Input::getKey(GLFW_KEY_R)) {
		camera.setPosition(normalPos);
		/*camera.setFront(normalFront);
		camera.setUp(normalUp);*/
		camera.setYaw(45.0f);
		camera.setPitch(0.0f);

		checkCurrentChunk = true;
	}

	// Virtual camera
	if (Input::getKeyDown(GLFW_KEY_C)) {
		camera.setLocked(!camera.getLocked());
		checkCurrentChunk = true;
	}

	// Collapse
	if (Input::getKeyDown(GLFW_KEY_Z)) {
		uiCollapsed = !uiCollapsed;
		debug.setCollapsed(uiCollapsed);
		debug.setSize(uiCollapsed ? 200 : 300, uiCollapsed ? 100 : windowHeight);
	}

	// Record
	if (Input::getKey(GLFW_KEY_F) && !recording) {
		camera.setPosition(normalPos);
		camera.setFront(normalFront);
		camera.setUp(normalUp);

		recording = true;
		recordingTimer = recordingTime;
		frameCounter = 0;
	}

	if (recording) {
		checkCurrentChunk = true;
	}

	// Render switch
	if (Input::getKeyDown(GLFW_KEY_T)) {
		if (world.getWireframeColour() == 0) {
			glPolygonMode(GL_FRONT, GL_LINE);
			world.setWireframeColour(1);
		} else {
			glPolygonMode(GL_FRONT, GL_FILL);
			world.setWireframeColour(0);
		}
	}

	// Speed
	float cameraSpeed = (Input::getKey(GLFW_KEY_LEFT_SHIFT) ? 10.0f : 5.0f) * deltaTime;
	camera.setSpeed(cameraSpeed);

	// Forward and backward
	if (Input::getKey(GLFW_KEY_W)) {
		camera.translate(camera.getFront());

		if (!camera.getLocked()) checkCurrentChunk = true;
	}
	if (Input::getKey(GLFW_KEY_S)) {
		camera.translate(-camera.getFront());

		if (!camera.getLocked()) checkCurrentChunk = true;
	}

	// Left and right
	if (Input::getKey(GLFW_KEY_A)) {
		camera.translate(-camera.getRight());

		if (!camera.getLocked()) checkCurrentChunk = true;
	}
	if (Input::getKey(GLFW_KEY_D)) {
		camera.translate(camera.getRight());

		if (!camera.getLocked()) checkCurrentChunk = true;
	}

	// Up and down
	if (Input::getKey(GLFW_KEY_E)) {
		camera.translate(camera.getRelativeUp());

		if (!camera.getLocked()) checkCurrentChunk = true;
	}
	if (Input::getKey(GLFW_KEY_Q)) {
		camera.translate(-camera.getRelativeUp());

		if (!camera.getLocked()) checkCurrentChunk = true;
	}

	if (checkCurrentChunk && backFaceCulling) {
		world.checkChunk(false);
	}
}

void toggleBackfaceCulling() {
	backFaceCulling = !backFaceCulling;
	if (!backFaceCulling) world.enableAllFaces();
	else world.checkChunk(true);
}

void regenerateWorld() {
	internalFaceCulling = !internalFaceCulling;
	world.clear();
	world.generate();
	if (internalFaceCulling) world.internalFaceCull();
}

int main(void) {
	auto start = std::chrono::high_resolution_clock::now();

	// Vertices
	float verDist = voxelSize / 2.0f;
	const float cubeVertices[] = {
		-verDist, -verDist, -verDist, 1.0f, 0.0f,
		 verDist, -verDist, -verDist, 0.0f, 0.0f,
		-verDist,  verDist, -verDist, 1.0f, 1.0f,
		 verDist,  verDist, -verDist, 0.0f, 1.0f
	};

	int numVertices = sizeof(cubeVertices) / sizeof(cubeVertices[0]);

	// Initialize renderer (GLFW / OpenGL)
	if (renderer.initialize(windowWidth, windowHeight, std::string(windowName + " - " + gameVersion), cubeVertices, numVertices) == -1)
		return -1;

	GLFWwindow* window = renderer.getWindow();

	// Debug window text
	debug.initialize(window);

	debug.addLine("[R] Reset the camera position");
	debug.addLine("[F] Start recording the performance");
	debug.addLine("[T] Look at the wireframes of the voxels");
	debug.addLine("[G] Look at the triangles of the voxels");
	debug.addLine("[C] Toggle locked camera");
	debug.addLine("");
	debug.addLine("[WASDQE] Move the camera");
	debug.addLine("[Mouse] Look around");
	debug.addLine("");
	debug.addButton("Toggle backface culling", &toggleBackfaceCulling);
	debug.addButton("Toggle internal face culling", &regenerateWorld);

	// Generate world
	world.generate();
	if (internalFaceCulling) world.internalFaceCull();

	// Shaders
	const char* vertexShaderSource = R"(
		#version 330 core
		layout(location = 0) in vec3 aPos;
		layout(location = 1) in vec2 aTexCoord;
		layout(location = 2) in vec3 aOffset;
		layout(location = 3) in mat4 aRotation;
		layout(location = 7) in int aId;

		out vec3 outColor;
		out int outId;
		out vec2 TexCoord;
		out vec4 outNormal;
		out vec3 outLightDir;
		out vec3 outCamPos;
		out vec2 outFogDis;
		out vec3 outSkyCol;
		out vec3 outWorldPos;

		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;
		uniform vec3 lightDir;
		uniform vec3 camPos;
		uniform vec2 fogDis;
		uniform vec3 skyCol;

		void main() {
			vec4 rotatedPos = aRotation * vec4(aPos, 1.0);
			gl_Position = projection * view * model * (rotatedPos + vec4(aOffset, 1.0));
			vec3 vertexColor = vec3(0.0, 0.0, 0.0);

			//if (aPos.x < 0.0) vertexColor.x++;
			//if (aPos.y > 0.0) vertexColor.y++;

			vec3 worldPos = gl_Position.xyz;
			vec4 normal = aRotation * vec4(0.0, 0.0, -1.0, 1.0);

			outWorldPos = worldPos;
			outNormal = normal;
			outId = aId;
			outColor = vertexColor;
			TexCoord = aTexCoord;
			outLightDir = lightDir;
			outCamPos = camPos;
			outFogDis = fogDis;
			outSkyCol = skyCol;
		}
	)";

	const char* fragmentShaderSource = R"(
		#version 330 core
		out vec4 FragColor;

		in vec3 outColor;
		in int outId;
		in vec2 TexCoord;
		in vec4 outNormal;
		in vec3 outLightDir;
		in vec3 outCamPos;
		in vec2 outFogDis;
		in vec3 outSkyCol;
		in vec3 outWorldPos;

		uniform sampler2D tex0;

		void main() {
			float dot = outNormal.x * -outLightDir.x + outNormal.y * -outLightDir.y + outNormal.z * -outLightDir.z;
			dot = (dot + 1.0) / 2.0;
			vec4 col = vec4(vec3(1.0, 1.0, 1.0) * dot, 1.0);

			float distance = distance(outCamPos, outWorldPos) - outFogDis.y;
			float lerp = clamp(distance / outFogDis.x, 0.0, 1.0);
			//col = vec4(col.xy, lerp, 1.0);
			col = vec4(vec3(col.rgb * (1.0 - lerp)), 1.0);
			col = col + vec4(outSkyCol * lerp, 0.0);

			//vec4 texColor = texture(tex0, TexCoord);
			//vec4 col = texColor + vec4(TexCoord.rg * 0.5, 0.0, 0.0);

			FragColor = col;
		}
	)";

	GLuint vertexShader = renderer.setShader(vertexShaderSource, GL_VERTEX_SHADER);
	GLuint fragmentShader = renderer.setShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
	GLuint shaderProgram = renderer.createShaderProgram(vertexShader, fragmentShader);
	world.setShaderProgram(shaderProgram);

	std::vector<std::string> filepaths = {
		"src/img/crate.png"
	};

	renderer.setupTextures(shaderProgram, filepaths);

	// MVP
	glm::mat4 view;
	glm::mat4 projection = glm::perspective(glm::radians(camera.getFov()), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;
	std::cout << "Execution time: " << duration.count() << " seconds\n";

	// Game loop
	while (!glfwWindowShouldClose(window)) {
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			std::cerr << "OpenGL error: " << err << std::endl;
		}

		// Delta time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Input and camera movement
		processInput(window);

		if (recording) {
			recordingTimer -= deltaTime;
			frameCounter++;
			fpsCounter += ImGui::GetIO().Framerate;

			if (recordingTimer <= 0) {
				recording = false;
				fpsCounter /= frameCounter;
				std::cout << "Amount of frames: " << frameCounter << "\nAverage FPS: " << fpsCounter << std::endl;
			}

			camera.translate(glm::vec3(deltaTime * recordCamSpeed, 0.0f, deltaTime * recordCamSpeed));
		}

		camera.update(deltaTime);

		// OpenGL clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(skyCol.x, skyCol.y, skyCol.z, 1.0f);

		// Shader uniforms
		glUseProgram(shaderProgram);
		GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
		GLuint wireLoc = glGetUniformLocation(shaderProgram, "wireframe");

		glUniform1i(wireLoc, wireframe);

		view = camera.getViewMatrix();
		
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		// Render world and debug window
		world.checkBlockManipulation();
		world.draw(skyCol);
		debug.draw();

		glfwSwapBuffers(window);

		// Update input
		Input::update();
		glfwPollEvents();
	}

	debug.destroy();
	glfwTerminate();
	return 0;
}