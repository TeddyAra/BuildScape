#include "Input.h"

GLFWwindow* Input::window = nullptr;
std::map<int, bool> Input::keyMemory;
glm::vec2 Input::mousePosition;
glm::vec2 Input::lastMousePosition;
bool Input::firstMouse = true;

void Input::setWindow(GLFWwindow* pWindow) {
	window = pWindow;
	glfwSetCursorPosCallback(window, Input::mouseCallback);
}

void Input::update() {
	std::map<int, bool>::iterator it;
	for (it = keyMemory.begin(); it != keyMemory.end(); it++) {
		keyMemory[it->first] = getKey(it->first);
	}

	lastMousePosition = mousePosition;
}

bool Input::getKey(int pKey) {
	return (glfwGetKey(window, pKey) == GLFW_PRESS);
}

bool Input::getKeyDown(int pKey) {
	if (keyMemory.find(pKey) != keyMemory.end()) {
		return (!keyMemory[pKey] && glfwGetKey(window, pKey) == GLFW_PRESS);
	} else {
		keyMemory[pKey] = false;
		return getKey(pKey);
	}
}

bool Input::getKeyUp(int pKey) {
	if (keyMemory.find(pKey) != keyMemory.end()) {
		return (keyMemory[pKey] && glfwGetKey(window, pKey) != GLFW_PRESS);
	} else {
		keyMemory[pKey] = false;
		return getKey(pKey);
	}
}

glm::vec2 Input::getMousePosition() {
	return mousePosition;
}

glm::vec2 Input::getDeltaMousePosition() {
	return mousePosition - lastMousePosition;
}

void Input::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		mousePosition = glm::vec2(xpos, ypos);
		firstMouse = false;
	}

	lastMousePosition = mousePosition;
	mousePosition = glm::vec2(xpos, ypos);
}