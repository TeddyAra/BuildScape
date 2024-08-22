#include "Renderer.h"

#include "vendor/stb_image/stb_image.h"

void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

// Logs all OpenGL errors, if any
bool GLLogCall(const char* function, const char* file, int line) {
    while (GLenum error = glGetError()) {
        std::cout << "[ERROR] OpenGL error (" << error << "): " << function << " " << file << ":" << line << std::endl;
        return false;
    }
    return true;
}

const unsigned int indices[] = {
    0, 2, 3,
    0, 3, 1
};

Renderer::Renderer()
	: VAO(0), VBO(0), EBO(0), instanceVBO(0), window(nullptr) {

}

Renderer::~Renderer() {
	
}

int Renderer::initialize(int pWindowWidth, int pWindowHeight, std::string pWindowName, const float pCubeVertices[], int pNumVertices) {
    // Initialize GLFW
    if (!glfwInit())
        return -1;

    // Create a window
    window = glfwCreateWindow(pWindowWidth, pWindowHeight, pWindowName.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    Input::setWindow(window);

    // GLFW settings
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, Input::mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize glew (OpenGL)
    if (glewInit() != GLEW_OK) {
        std::cout << "[ERROR] Initializing glew failed." << std::endl;
        return -1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // OpenGL settings
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_TEXTURE_2D);

    // Generate buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &instanceVBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_STATIC_DRAW);

    // Bind the voxel vertex array
    glBindVertexArray(VAO);

    // Bind and buffer cube vertices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, pNumVertices * sizeof(float), pCubeVertices, GL_STATIC_DRAW);

    // Vertex position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // UV position attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Bind and buffer instance data
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Instance offset attribute 
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Chunk::InstanceData), (void*)(offsetof(Chunk::InstanceData, offset)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Instance rotation attribute 
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(Chunk::InstanceData), (void*)(offsetof(Chunk::InstanceData, rotation) + i * sizeof(glm::vec4)));
        glEnableVertexAttribArray(3 + i);
        glVertexAttribDivisor(3 + i, 1);
    }

    // Instance id attribute
    glVertexAttribPointer(7, 1, GL_INT, GL_FALSE, sizeof(Chunk::InstanceData), (void*)(offsetof(Chunk::InstanceData, id)));
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);

    return 0;
}


GLFWwindow* Renderer::getWindow() {
	return window;
}

void Renderer::bindVAO() {
    glBindVertexArray(VAO);
}

void Renderer::unbindVAO() {
    glBindVertexArray(0);
}

void Renderer::bindEBO() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
}

void Renderer::unbindEBO() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::bindInstanceVBO() {
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
}

void Renderer::unbindInstanceVBO() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::setupTextures(GLuint pShaderProgram, const std::vector<std::string>& pFilepaths) {
    glUseProgram(pShaderProgram);

    for (int i = 0; i < pFilepaths.size(); i++) {
        Texture texture(pFilepaths[i], i);
        texture.bind(i);
        textures.push_back(texture);

        GLuint textureLoc = glGetUniformLocation(pShaderProgram, ("tex" + std::to_string(i)).c_str());
        glUniform1i(textureLoc, i);

        std::cout << "Setup texture " << pFilepaths[i] << " to tex" << i << std::endl;
    }
}

void Renderer::setTextureUniforms(GLuint pShaderProgram) {
    for (int i = 0; i < textures.size(); i++) {
        GLuint textureLoc = glGetUniformLocation(pShaderProgram, ("tex" + std::to_string(i)).c_str());
        textures[i].bind();
        glUniform1i(textureLoc, i);
    }
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