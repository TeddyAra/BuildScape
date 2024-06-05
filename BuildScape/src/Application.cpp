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

struct Chunk {
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

	Chunk(int x, int y, int z) {
		posX = x;
		posY = y;
		posZ = z;
	}

	// Blocks in chunk
	std::vector<std::uint32_t> blocks;

	// Chunk position
	int posX;
	int posY;
	int posZ;

	bool empty = true;

	bool ignoreLeft =	false;
	bool ignoreRight =	false;
	bool ignoreDown =	false;
	bool ignoreUp =		false;
	bool ignoreFront =	false;
	bool ignoreBack =	false;

	void AddBlock(std::uint32_t block) {
		blocks.push_back(block);
		empty = false;
	}

	bool operator==(const Chunk& chunk) {
		return (posX == chunk.posX &&
				posY == chunk.posY &&
				posZ == chunk.posZ);
	}

	bool operator!=(const Chunk& chunk) {
		return (posX != chunk.posX ||
				posY != chunk.posY ||
				posZ != chunk.posZ);
	}
};

std::vector<Chunk> chunks;

// Camera
glm::vec3 normalPos = glm::vec3(-2.0f, 8.0f, -2.0f);
glm::vec3 normalFront = glm::normalize(glm::vec3(1.0f, -0.5f, 1.0f));
glm::vec3 normalUp = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec3 cameraPos = normalPos;
glm::vec3 cameraFront = normalFront;
glm::vec3 cameraUp = normalUp;

glm::vec3 virtualPos;
glm::vec3 virtualFront;
glm::vec3 virtualUp;
bool usingVirtualCamera;

glm::vec3 closestChunkPos;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400;
float lastY = 300;
bool firstMouse = true;

int wireframe = 0;
bool recording = false;
float recordingTime = 10;
float recordingTimer = 0;
float recordCamSpeed = 3;
int frameCounter = 0;
bool uiCollapsed = false;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; 
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

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

void internalFaceCulling(Chunk& chunk) {
    std::vector<std::uint32_t>& blocks = chunk.blocks;

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

// Input
void processInput(GLFWwindow* window) {
	// Reset
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		cameraPos = normalPos;
		cameraFront = normalFront;
		cameraUp = normalUp;

		checkCurrentChunk = true;
	}

	// Virtual camera
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		virtualPos = cameraPos;
		virtualFront = cameraFront;
		virtualUp = cameraUp;

		usingVirtualCamera = true;
	}

	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
		usingVirtualCamera = false;

		checkCurrentChunk = true;
	}

	// Collapse
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		uiCollapsed = true;
	}

	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
		uiCollapsed = false;
	}

	// Record
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !recording) {
		cameraPos = normalPos;
		cameraFront = normalFront;
		cameraUp = normalUp;

		recording = true;
		recordingTimer = recordingTime;
		frameCounter = 0;
	}

	if (recording) {
		checkCurrentChunk = true;
	}

	// Render switch
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		wireframe = 1;
	}

	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		wireframe = 0;
	}

	// Speed
	float cameraSpeed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 10.0f : 5.0f) * deltaTime; 

	// Forward and backward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		cameraPos += cameraSpeed * cameraFront;

		if (!usingVirtualCamera) checkCurrentChunk = true;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		cameraPos -= cameraSpeed * cameraFront;

		if (!usingVirtualCamera) checkCurrentChunk = true;
	}

	// Left and right
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

		if (!usingVirtualCamera) checkCurrentChunk = true;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

		if (!usingVirtualCamera) checkCurrentChunk = true;
	}

	// Up and down
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		cameraPos += glm::normalize(glm::cross(glm::cross(cameraFront, cameraUp), cameraFront)) * cameraSpeed;

		if (!usingVirtualCamera) checkCurrentChunk = true;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		cameraPos -= glm::normalize(glm::cross(glm::cross(cameraFront, cameraUp), cameraFront)) * cameraSpeed;

		if (!usingVirtualCamera) checkCurrentChunk = true;
	}

	if (checkCurrentChunk && BACK_FACE_CULLING) {
		checkCurrentChunk = false;
		glm::vec3 pos = usingVirtualCamera ? virtualPos : cameraPos;

		for (Chunk& chunk : chunks) {
			if (pos.x > chunk.posX && pos.x < chunk.posX + 16 * blockSize &&
				pos.y > chunk.posY && pos.y < chunk.posY + 16 * blockSize &&
				pos.z > chunk.posZ && pos.z < chunk.posZ + 16 * blockSize) {

				glm::vec3 newClosest(chunk.posX, chunk.posY, chunk.posZ);

				if (newClosest != closestChunkPos) {
					closestChunkPos = newClosest;

					chunk.ignoreRight = false;
					chunk.ignoreLeft = false;
					chunk.ignoreUp = false;
					chunk.ignoreDown = false;
					chunk.ignoreFront = false;
					chunk.ignoreBack = false;

					for (Chunk& chunkCopy : chunks) {
						if (glm::vec3(chunkCopy.posX, chunkCopy.posY, chunkCopy.posZ) == closestChunkPos || chunkCopy.empty) continue;

						chunkCopy.ignoreLeft =	chunkCopy.posX < closestChunkPos.x;
						chunkCopy.ignoreRight = chunkCopy.posX > closestChunkPos.x;
						chunkCopy.ignoreDown =	chunkCopy.posY < closestChunkPos.y;
						chunkCopy.ignoreUp =	chunkCopy.posY > closestChunkPos.y;
						chunkCopy.ignoreBack =	chunkCopy.posZ > closestChunkPos.z;
						chunkCopy.ignoreFront = chunkCopy.posZ < closestChunkPos.z;
					}
				}

				break;
			}
		}
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

	glfwMakeContextCurrent(window);
	//glfwSwapInterval(1);
	glfwSetCursorPosCallback(window, mouse_callback);
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

								chunk.AddBlock(block);
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
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

	// Game loop
	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		if (recording) {
			recordingTimer -= deltaTime;
			frameCounter++;

			if (recordingTimer <= 0) {
				recording = false;
				std::cout << "Amount of frames: " << frameCounter << std::endl;
			}

			cameraPos += glm::vec3(deltaTime * recordCamSpeed, 0.0f, deltaTime * recordCamSpeed);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
		GLuint wireLoc = glGetUniformLocation(shaderProgram, "wireframe");

		glUniform1i(wireLoc, wireframe);

		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(VAO);

		// For each block in the chunk, apply a model transformation and draw it
		for (Chunk chunk : chunks) {
			if (chunk.empty) continue;

			for (const auto& block : chunk.blocks) {
				int id = (block >> 12) & 0xFF;
				if (id == 0) continue;

				int x = (block >> 28) & 0x0F;
				int y = (block >> 24) & 0x0F;
				int z = (block >> 20) & 0x0F;

				glm::vec3 pos(glm::vec3(x * blockSize + chunk.posX, y * blockSize + chunk.posY, z * blockSize + chunk.posZ));

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

				if (left == 0 && !chunk.ignoreLeft) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesLeft), std::end(cubeIndicesLeft));
				}
				if (right == 0 && !chunk.ignoreRight) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesRight), std::end(cubeIndicesRight));
				}
				if (up == 0 && !chunk.ignoreUp) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesTop), std::end(cubeIndicesTop));
				}
				if (down == 0 && !chunk.ignoreDown) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesBottom), std::end(cubeIndicesBottom));
				}
				if (front == 0 && !chunk.ignoreFront) {
					combinedIndices.insert(combinedIndices.end(), std::begin(cubeIndicesFront), std::end(cubeIndicesFront));
				}
				if (back == 0 && !chunk.ignoreBack) {
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
			ImGui::Text("[X] Uncollapse the UI");
		} else {
			ImGui::Text("[Z] Collapse the UI");
			ImGui::Text("[R] Reset the camera position");
			ImGui::Text("[F] Start recording the performance");
			ImGui::Text("[T] Look at the wireframes of the voxels");
			ImGui::Text("[G] Look at the triangles of the voxels");
			ImGui::Text(usingVirtualCamera ? "[V] Turn virtual camera off" : "[C] Turn virtual camera on");
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

		glfwPollEvents();
	}

	ImGui_ImplGlfwGL3_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}