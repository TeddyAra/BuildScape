#pragma once

#include <vector>
#include "glm/glm.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

class Debug {
public:
	Debug(const char* pDebugName, int pWidth, int pHeight);
	~Debug();

	void initialize(GLFWwindow* pWindow);
	void draw();

	void setSize(int pWidth, int pHeight);
	void setCollapsed(bool pCollapsed);
	void addLine(const char* pLine);

private:
	const char* debugName;
	glm::vec2 debugSize;
	std::vector<const char*> lines;
	bool collapsed;
};