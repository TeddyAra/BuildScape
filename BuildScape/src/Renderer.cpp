#include "Renderer.h"

Renderer::Renderer()
	: VAO(0), VBO(0), EBO(0), window(nullptr) {

}

Renderer::~Renderer() {

}

int Renderer::initialize(int pWindowWidth, int pWindowHeight, std::string(pWindowName), const float pCubeVertices[], int pNumVertices) {
	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(pWindowWidth, pWindowHeight, pWindowName.c_str(), NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}
	Input::setWindow(window);

	glfwMakeContextCurrent(window);
	//glfwSwapInterval(1);
	glfwSetCursorPosCallback(window, Input::mouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (glewInit() != GLEW_OK) {
		std::cout << "[ERROR] Initializing glew failed." << std::endl;
	}

	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, pNumVertices * sizeof(float), pCubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	return 0;
}

GLFWwindow* Renderer::getWindow() {
	return window;
}

void Renderer::bindEBO() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
}

void Renderer::unbindEBO() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLuint Renderer::setShader(const char* pShaderSource, GLenum pType) {
	GLuint shader = glCreateShader(pType);
	glShaderSource(shader, 1, &pShaderSource, nullptr);
	glCompileShader(shader);
	return shader;
}

GLuint Renderer::createShaderProgram(GLuint pVertexShader, GLuint pFragmentShader) {
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, pVertexShader);
	glAttachShader(shaderProgram, pFragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(pVertexShader);
	glDeleteShader(pFragmentShader);

	return shaderProgram;
}