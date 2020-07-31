#pragma once

#include <GLFW/glfw3.h>
namespace Input
{
	void handleMouseScroll(GLFWwindow *window, double xoffset, double yoffset);
	void handleKeyAction(GLFWwindow *window, int key, int scancode, int action, int mods);
	void handleCursorPosition(GLFWwindow *window, double x, double y);
	void handleMouseKeyAction(GLFWwindow *window, int button, int action, int mods);
	void update(GLFWwindow *window);

}; // namespace Input