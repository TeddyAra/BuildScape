#pragma once

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include "Input.h"

class Renderer {
public:
	Renderer();
	~Renderer();

	int initialize(int pWindowWidth, int pWindowHeight, std::string(pWindowName), const float pCubeVertices[], int pNumVertices);
	GLFWwindow* getWindow();
	void bindEBO();
	void unbindEBO();

	GLuint setShader(const char* pShaderSource, GLenum pType);
	GLuint createShaderProgram(GLuint pVertexShader, GLuint pFragmentShader);

private:
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;

	GLFWwindow* window;
};