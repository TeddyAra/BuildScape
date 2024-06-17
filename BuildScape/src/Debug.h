#pragma once

#include <vector>
#include "glm/glm.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"
#include <map>
#include <functional>

class Debug {
public:
	Debug(const char* pDebugName, int pWidth, int pHeight);
	~Debug();

	void initialize(GLFWwindow* pWindow);
	void destroy();
	void draw();

	void setSize(int pWidth, int pHeight);
	void setCollapsed(bool pCollapsed);
	void addLine(const char* pLine);
	void addButton(const char* pLine, std::function<void()> pFunction);

private:
	const char* debugName;
	glm::vec2 debugSize;
	std::vector<const char*> lines;
	std::map<const char*, std::function<void()>> buttons;
	bool collapsed;
};