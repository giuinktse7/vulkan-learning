#pragma once

#include <GLFW/glfw3.h>
#include <bitset>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <ostream>
#include <array>
#include <vector>
#include <initializer_list>

#include "time.h"
#include "position.h"

enum class KeyState
{
	Release,
	Pressed,
	JustPressed,
	Repeat
};

struct KeyCombination
{
	KeyCombination(int key);

	int key;
	bool ctrl;
	bool shift;
	bool alt;
};

class Input
{
	using FunctionPtr = void (*)(Input *);
	using GLFWKey = int;

public:
	Input(GLFWwindow *window);
	void handleScrollCallback(double xoffset, double yoffset);
	void handleKeyCallback(GLFWKey key, int scancode, int action, int mods);
	void handleCursorPosCallback(double x, double y);
	void handleMouseButtonCallback(int button, int action, int mods);
	void update();
	void registerHook(FunctionPtr ptr);

	bool released(GLFWKey key) const;
	/*
		Pressed or repeated
	*/
	bool down(GLFWKey key) const;
	bool pressed(GLFWKey key) const;
	bool justPressed(GLFWKey key) const;
	bool pressed(const KeyCombination key) const;
	bool repeated(GLFWKey key) const;
	bool anyRepeated(std::initializer_list<GLFWKey> keys) const;

	bool ctrl() const;
	bool shift() const;
	bool alt() const;

	KeyState getKeyState(GLFWKey GLFWKey) const;

	void setKeyState(GLFWKey GLFWKey, int state);

	std::pair<double, double> scrollOfset() const;

	bool leftMouseDown() const;
	bool rightMouseDown() const;

	/*
		Returns the key event for the key if it was the latest changed key and it has a
		non-consumed event (events are consumed at the end of each call to update).
		Returns -1 if the key was not the latest changed key.
	*/
	int keyEvent(GLFWKey key);

	int leftMouseEvent() const;
	int rightMouseEvent() const;

	bool keyDownEvent(GLFWKey key);

	ScreenPosition cursorPos();

	TimePoint lastUpdateTime;

private:
	GLFWwindow *window;
	std::array<GLFWKey, GLFW_KEY_LAST> keyState{GLFW_RELEASE};

	std::unordered_set<GLFWKey> justPressedState;

	std::unordered_set<GLFWKey> pressedKeys;

	std::unordered_set<FunctionPtr> inputHooks;

	int currentLeftMouseEvent;
	int currentRightMouseEvent;

	struct CurrentKeyChange
	{
		GLFWKey key;
		int state;
	} currentKeyChange;

	struct CurrentScrollOffset
	{
		double x;
		double y;
	} currentScrollOffset;

	ScreenPosition currentCursorPos;
};

inline std::ostream &operator<<(std::ostream &os, KeyState type)
{
	switch (type)
	{
	case KeyState::Release:
		os << "KeyState::Release";
		break;
	case KeyState::Pressed:
		os << "KeyState::Pressed";
		break;
	case KeyState::JustPressed:
		os << "KeyState::JustPressed";
		break;
	case KeyState::Repeat:
		os << "KeyState::Repeat";
		break;
	}

	return os;
}
