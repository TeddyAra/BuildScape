#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

#include <iostream>
#include <vector>
#include <stdint.h>
#include <cstdint>
#include <random>

#include "Input.h"
#include "Camera.h"
#include "Chunk.h"

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

const bool INTERNAL_FACE_CULLING = true;
const bool BACK_FACE_CULLING = true;

float blockSize = 0.5f;

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());

int random(int min, int max) {
	std::uniform_int_distribution<> num(min, max);
	return (int)num(gen);
}

// Camera
glm::vec3 normalPos = glm::vec3(-2.0f, 8.0f, -2.0f);
glm::vec3 normalFront = glm::normalize(glm::vec3(1.0f, -0.5f, 1.0f));
glm::vec3 normalUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 closestChunkPos;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool firstMouse = true;

int wireframe = 0;
bool recording = false;
float recordingTime = 10;
float recordingTimer = 0;
float recordCamSpeed = 3;
int frameCounter = 0;
bool uiCollapsed = false;

GLuint VAO, VBO, EBO;

const unsigned int cubeIndicesLeft[] = {
	4, 6, 2,
	4, 2, 0
};

const unsigned int cubeIndicesRight[] = {
	1, 3, 7,
	1, 7, 5
};

const unsigned int cubeIndicesTop[] = {
	2, 6, 7,
	2, 7, 3
};

const unsigned int cubeIndicesBottom[] = {
	4, 0, 1,
	4, 1, 5
};

const unsigned int cubeIndicesFront[] = {
	0, 2, 3,
	0, 3, 1
};

const unsigned int cubeIndicesBack[] = {
	4, 6, 7,
	4, 7, 5
};

int isNeighborPresent(const std::vector<std::uint32_t>& blocks, int index, int dir) {
	std::uint32_t block = blocks[index];
	int x = (block >> 28) & 0x0F;
	int y = (block >> 24) & 0x0F;
	int z = (block >> 20) & 0x0F;

	switch (dir) {
	case 0: // Left
		if (x == 0) return 0;
		index -= 1;
		break;
	case 1: // Right
		if (x == 15) return 0;
		index += 1;
		break;
	case 2: // Down
		if (y == 0) return 0;
		index -= 256;
		break;
	case 3: // Up
		if (y == 15) return 0;
		index += 256;
		break;
	case 4: // Front
		if (z == 0) return 0;
		index -= 16;
		break;
	case 5: // Back
		if (z == 15) return 0;
		index += 16;
		break;
	}

	block = blocks[index];
	int id = (block >> 12) & 0xFF;
	return id == 0 ? 0 : 1;
}

void internalFaceCulling(Chunk& pChunk) {
    std::vector<std::uint32_t>& blocks = pChunk.getBlocks();

    for (size_t i = 0; i < blocks.size(); ++i) {
        std::uint32_t block = blocks[i];
        int id = (block >> 12) & 0xFF;
        if (id == 0) continue;

        std::uint32_t blockCopy = block;

        blockCopy &= ~(0x3F << 6); 
        for (int j = 0; j < 6; ++j) {
            if (isNeighborPresent(blocks, i, j)) {
                blockCopy |= (1 << (11 - j));
            }
        }

        blocks[i] = blockCopy;
    }
}

bool checkCurrentChunk = true;

Camera camera(normalPos, normalFront, normalUp, 1.0f, 45.0f, 1.0f);

void checkChunk() {
	checkCurrentChunk = false;
	glm::vec3 pos = camera.getLocked() ? camera.getLockedPosition() : camera.getPosition();

	for (Chunk& chunk : chunks) {
		if (pos.x > chunk.getPosition().x && pos.x < chunk.getPosition().x + 16 * blockSize &&
			pos.y > chunk.getPosition().y && pos.y < chunk.getPosition().y + 16 * blockSize &&
			pos.z > chunk.getPosition().z && pos.z < chunk.getPosition().z + 16 * blockSize) {

			glm::vec3 newClosest = chunk.getPosition();

			if (newClosest != closestChunkPos) {
				closestChunkPos = newClosest;

				chunk.setIgnoreRight(false);
				chunk.setIgnoreLeft(false);
				chunk.setIgnoreUp(false);
				chunk.setIgnoreDown(false);
				chunk.setIgnoreFront(false);
				chunk.setIgnoreBack(false);

				for (Chunk& chunkCopy : chunks) {
					if (chunk.getPosition() == closestChunkPos || chunkCopy.isEmpty()) continue;

					chunkCopy.setIgnoreLeft(chunkCopy.getPosition().x < closestChunkPos.x);
					chunkCopy.setIgnoreRight(chunkCopy.getPosition().x > closestChunkPos.x);
					chunkCopy.setIgnoreDown(chunkCopy.getPosition().y < closestChunkPos.y);
					chunkCopy.setIgnoreUp(chunkCopy.getPosition().y > closestChunkPos.y);
					chunkCopy.setIgnoreBack(chunkCopy.getPosition().z > closestChunkPos.z);
					chunkCopy.setIgnoreFront(chunkCopy.getPosition().z < closestChunkPos.z);
				}
			}

			break;
		}
	}
}

// Input
void processInput(GLFWwindow* window) {
	// Reset
	if (Input::getKey(GLFW_KEY_R)) {
		camera.setPosition(normalPos);
		camera.setFront(normalFront);
		camera.setUp(normalUp);

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
	if (Input::getKey(GLFW_KEY_T)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		wireframe = 1;
	}

	if (Input::getKey(GLFW_KEY_G)) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		wireframe = 0;
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

	if (checkCurrentChunk && BACK_FACE_CULLING) {
		checkChunk();
	}
}

int main(void) {
	// Game information
	std::string windowName = "BuildScape";
	std::string gameVersion = "Alpha";

	// Window variables
	int windowWidth = 960;
	int windowHeight = 540;

	// Game variables
	int test = 4;

	float verDist = blockSize / 2;
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

	GLFWwindow* window;

	// Initialize glfw
	if (!glfwInit())
		return -1;

	// Create a window
	window = glfwCreateWindow(windowWidth, windowHeight, std::string(windowName + " - " + gameVersion).c_str(), NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	Input::setWindow(window);

	glfwMakeContextCurrent(window);
	//glfwSwapInterval(1);
	glfwSetCursorPosCallback(window, Input::mouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize glew
	if (glewInit() != GLEW_OK) {
		std::cout << "[ERROR] Initializing glew failed." << std::endl;
	}

	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Block creation
	for (int cZ = -6; cZ < 6; cZ++) {
		for (int cY = -4; cY < 5; cY++) {
			for (int cX = -6; cX < 6; cX++) {
				Chunk chunk(cX * 16 * blockSize, cY * 16 * blockSize, cZ * 16 * blockSize);

				if (cY == 0 && cX > -2 && cX < 3 && cZ > -2 && cZ < 3) {
					for (int y = 0; y < 16; y++) {
						for (int z = 0; z < 16; z++) {
							for (int x = 0; x < 16; x++) {
								std::uint32_t block = 0;

								// Position
								block |= (x << 28);
								block |= (y << 24);
								block |= (z << 20);

								// ID
								int air = 0;
								if (y < test - 1) air = 1;
								if (y == test - 1) air = random(0, 1);

								block |= (air << 12);

								chunk.addBlock(block);
							}
						}
					}

					if (INTERNAL_FACE_CULLING) internalFaceCulling(chunk);
				}

				chunks.push_back(chunk);
			}
		}
	}

	ImGui::CreateContext();
	ImGui_ImplGlfwGL3_Init(window, true);
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 titleColor = ImVec4(0.15f, 0.3f, 0.5f, 1.0f);

	style.Colors[ImGuiCol_TitleBg] = titleColor;
	style.Colors[ImGuiCol_TitleBgActive] = titleColor;
	style.Colors[ImGuiCol_TitleBgCollapsed] = titleColor;

	// Arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndicesFront), cubeIndicesFront, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

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

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// MVP
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view;
	glm::mat4 projection = glm::perspective(glm::radians(camera.getFov()), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

	// Game loop
	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);
		camera.update(deltaTime);

		if (recording) {
			recordingTimer -= deltaTime;
			frameCounter++;

			if (recordingTimer <= 0) {
				recording = false;
				std::cout << "Amount of frames: " << frameCounter << std::endl;
			}

			camera.translate(glm::vec3(deltaTime * recordCamSpeed, 0.0f, deltaTime * recordCamSpeed));
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
		GLuint wireLoc = glGetUniformLocation(shaderProgram, "wireframe");

		glUniform1i(wireLoc, wireframe);

		view = camera.getViewMatrix();
		
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(VAO);

		// For each block in the chunk, apply a model transformation and draw it
		for (Chunk chunk : chunks) {
			if (chunk.isEmpty()) continue;

			for (const auto& block : chunk.getBlocks()) {
				int id = (block >> 12) & 0xFF;
				if (id == 0) continue;

				int x = (block >> 28) & 0x0F;
				int y = (block >> 24) & 0x0F;
				int z = (block >> 20) & 0x0F;

				glm::vec3 pos(glm::vec3(x * blockSize, y * blockSize, z * blockSize) * chunk.getPosition());

				int left = 0;
				int right = 0;
				int down = 0;
				int up = 0;
				int front = 0;
				int back = 0;

				if (INTERNAL_FACE_CULLING) {
					left = (block >> 11) & 0x01;
					right = (block >> 10) & 0x01;
					down = (block >> 9) & 0x01;
					up = (block >> 8) & 0x01;
					front = (block >> 7) & 0x01;
					back = (block >> 6) & 0x01;
				}

				model = glm::translate(glm::mat4(1.0f), pos);
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

				GLuint colLoc = glGetUniformLocation(shaderProgram, "col");
				glUniform3f(colLoc, wireframe, wireframe, wireframe);

				std::vector<GLuint> combinedIndices;

				if (left == 0 && !chunk.getIgnoreLeft()) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesLeft), std::end(cubeIndicesLeft));
				}
				if (right == 0 && !chunk.getIgnoreRight()) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesRight), std::end(cubeIndicesRight));
				}
				if (up == 0 && !chunk.getIgnoreUp()) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesTop), std::end(cubeIndicesTop));
				}
				if (down == 0 && !chunk.getIgnoreDown()) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesBottom), std::end(cubeIndicesBottom));
				}
				if (front == 0 && !chunk.getIgnoreFront()) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesFront), std::end(cubeIndicesFront));
				}
				if (back == 0 && !chunk.getIgnoreBack()) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesBack), std::end(cubeIndicesBack));
				}

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, combinedIndices.size() * sizeof(GLuint), combinedIndices.data(), GL_STATIC_DRAW);

				glDrawElements(GL_TRIANGLES, combinedIndices.size(), GL_UNSIGNED_INT, 0);
			}
		}

		glBindVertexArray(0);

		ImGui_ImplGlfwGL3_NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		if (uiCollapsed) 
			ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Always);
		else
			ImGui::SetNextWindowSize(ImVec2(300, windowHeight), ImGuiCond_Always);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		ImGui::Begin("Debug information", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
		if (uiCollapsed) {
			ImGui::Text("[Z] Uncollapse the UI");
		} else {
			ImGui::Text("[Z] Collapse the UI");
			ImGui::Text("[R] Reset the camera position");
			ImGui::Text("[F] Start recording the performance");
			ImGui::Text("[T] Look at the wireframes of the voxels");
			ImGui::Text("[G] Look at the triangles of the voxels");
			ImGui::Text(camera.getLocked() ? "[C] Turn locked camera off" : "[C] Turn locked camera on");
			ImGui::Text("");
			ImGui::Text("[WASDQE] Move the camera");
			ImGui::Text("[Mouse] Look around");
		}
		ImGui::Text("");
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::End();

		ImGui::PopStyleVar();

		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		Input::update();

		glfwPollEvents();
	}

	ImGui_ImplGlfwGL3_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}