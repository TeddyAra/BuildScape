#pragma once

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include <cstddef>
#include "Input.h"
#include "Chunk.h"

class Renderer {
public:
	Renderer();
	~Renderer();

	int initialize(int pWindowWidth, int pWindowHeight, std::string(pWindowName), const float pCubeVertices[], int pNumVertices);
	GLFWwindow* getWindow();

	void bindVAO();
	void unbindVAO();

	void bindEBO();
	void unbindEBO();

	void bindInstanceVBO();
	void unbindInstanceVBO();

	GLuint setShader(const char* pShaderSource, GLenum pType);
	GLuint createShaderProgram(GLuint pVertexShader, GLuint pFragmentShader);

private:
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;
	GLuint instanceVBO;

	GLFWwindow* window;
};