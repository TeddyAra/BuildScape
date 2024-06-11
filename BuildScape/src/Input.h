#pragma once

#include <string>
#include <iostream>
#include <map>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Input {
public:
	Input() = delete;

	static void setWindow(GLFWwindow* pWindow);
	static void update();

	// Keys
	static bool getKey(int pKey);
	static bool getKeyDown(int pKey);
	static bool getKeyUp(int pKey);

	// Mouse
	static glm::vec2 getMousePosition();
	static glm::vec2 getDeltaMousePosition();
	static void mouseCallback(GLFWwindow* window, double xPos, double yPos);

private:
	static std::map<int, bool> keyMemory;
	static GLFWwindow* window;

	static glm::vec2 mousePosition;
	static glm::vec2 lastMousePosition;
	static bool firstMouse;
};