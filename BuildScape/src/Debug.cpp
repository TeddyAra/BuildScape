#include "Debug.h"

Debug::Debug(const char* pDebugName, int pWidth, int pHeight)
	: debugName(pDebugName), debugSize(glm::vec2(pWidth, pHeight)), collapsed(false)
{

}

Debug::~Debug() {

}

void Debug::initialize(GLFWwindow* pWindow) {
	// Initialize GLFW
	ImGui::CreateContext();
	ImGui_ImplGlfwGL3_Init(pWindow, true);

	// Create style
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 titleColor = ImVec4(0.15f, 0.3f, 0.5f, 1.0f);

	style.Colors[ImGuiCol_TitleBg] = titleColor;
	style.Colors[ImGuiCol_TitleBgActive] = titleColor;
	style.Colors[ImGuiCol_TitleBgCollapsed] = titleColor;
}

void Debug::destroy() {
	// Destroy GLFW 
	ImGui_ImplGlfwGL3_Shutdown();
	ImGui::DestroyContext();
}

void Debug::draw() {
	// Start a new frame
	ImGui_ImplGlfwGL3_NewFrame();

	// Make window ready
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(debugSize.x, debugSize.y), ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::Begin(debugName, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	// Window text
	if (collapsed) {
		ImGui::Text("[Z] Uncollapse the UI");
	} else {
		ImGui::Text("[Z] Collapse the UI");
		for (const char* line : lines) {
			ImGui::Text(line);
		}
		for (const auto& iterator : buttons) {
			if (ImGui::Button(iterator.first, ImVec2(250, 20))) {
				iterator.second();
			}
		}
	}
	ImGui::Text("");
	ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
	ImGui::End();

	ImGui::PopStyleVar();

	// Show debug window
	ImGui::Render();
	ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
}

void Debug::setSize(int pWidth, int pHeight) {
	debugSize = glm::vec2(pWidth, pHeight);
}

void Debug::setCollapsed(bool pCollapsed) {
	collapsed = pCollapsed;
}

void Debug::addLine(const char* pLine) {
	lines.push_back(pLine);
}

void Debug::addButton(const char* pLine, std::function<void()> pFunction) {
	buttons[pLine] = pFunction;
}