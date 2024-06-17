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

bool internalFaceCulling = false;
bool backFaceCulling = true;

// Game variables
std::string windowName = "BuildScape";
std::string gameVersion = "Alpha";

int windowWidth = 960;
int windowHeight = 540;
float voxelSize = 0.5f;

glm::vec3 normalPos = glm::vec3(-2.0f, 8.0f, -2.0f);
glm::vec3 normalFront = glm::normalize(glm::vec3(1.0f, -0.5f, 1.0f));
glm::vec3 normalUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Classes
Camera camera(normalPos, normalFront, normalUp, 1.0f, 45.0f, 1.0f);
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
		-verDist, -verDist, -verDist,
		 verDist, -verDist, -verDist,
		-verDist,  verDist, -verDist,
		 verDist,  verDist, -verDist,
		-verDist, -verDist,  verDist,
		 verDist, -verDist,  verDist,
		-verDist,  verDist,  verDist,
		 verDist,  verDist,  verDist
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

		out vec3 outColor;

		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

		uniform vec3 col;

		void main() {
			gl_Position = projection * view * model * vec4(aPos, 1.0);
			vec3 vertexColor = col;

			if (aPos.x > 0.0) vertexColor.x++;
			if (aPos.y > 0.0) vertexColor.y++;
			if (aPos.z > 0.0) vertexColor.z++;

			outColor = vertexColor;
		}
	)";

	const char* fragmentShaderSource = R"(
		#version 330 core
		out vec4 FragColor;

		in vec3 outColor;

		void main() {
			FragColor = vec4(outColor, 1.0);
		}
	)";

	GLuint vertexShader = renderer.setShader(vertexShaderSource, GL_VERTEX_SHADER);
	GLuint fragmentShader = renderer.setShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
	GLuint shaderProgram = renderer.createShaderProgram(vertexShader, fragmentShader);
	world.setShaderProgram(shaderProgram);

	// MVP
	glm::mat4 view;
	glm::mat4 projection = glm::perspective(glm::radians(camera.getFov()), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;
	std::cout << "Execution time: " << duration.count() << " seconds\n";

	// Game loop
	while (!glfwWindowShouldClose(window)) {
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
		world.draw();
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