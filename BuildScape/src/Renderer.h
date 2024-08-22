#pragma once

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include <cstddef>
#include "Input.h"
#include "Chunk.h"
#include "Texture.h"

#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

void GLClearError();

bool GLLogCall(const char* function, const char* file, int line);

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

	void setupTextures(GLuint pShaderProgram, const std::vector<std::string>& pFilepaths);
	void setTextureUniforms(GLuint pShaderProgram);

	GLuint setShader(const char* pShaderSource, GLenum pType);
	GLuint createShaderProgram(GLuint pVertexShader, GLuint pFragmentShader);

private:
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;
	GLuint instanceVBO;

	std::vector<Texture> textures;

	GLFWwindow* window;
};