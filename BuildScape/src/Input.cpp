#include "Input.h"

GLFWwindow* Input::window = nullptr;
std::map<int, bool> Input::keyMemory;
glm::vec2 Input::mousePosition;
glm::vec2 Input::lastMousePosition;
bool Input::firstMouse = true;
bool Input::ignoreMouse = false;

void Input::setWindow(GLFWwindow* pWindow) {
	window = pWindow;
	glfwSetCursorPosCallback(window, Input::mouseCallback);
}

void Input::update() {
	// Update the map to hold the last frame's key and mouse information
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
	// Do nothing if we're ignoring the mouse
	if (ignoreMouse) return;

	// If this is called for the first time, have mousePosition set to the current mouse position
	if (firstMouse) {
		mousePosition = glm::vec2(xpos, ypos);
		firstMouse = false;
	}

	// Update current mouse position
	mousePosition = glm::vec2(xpos, ypos);
}

void Input::toggleIgnoreMouse() {
	ignoreMouse = !ignoreMouse;
	glfwSetInputMode(window, GLFW_CURSOR, ignoreMouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	//firstMouse = ignoreMouse;
}