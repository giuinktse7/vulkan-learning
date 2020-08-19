#include "input.h"

#include <algorithm>

#include "graphics/engine.h"
#include "ecs/ecs.h"
#include "ecs/item_selection.h"
#include "util.h"

Input::Input(GLFWwindow *window)
    : window(window)
{

  glfwSetWindowUserPointer(window, this);

  glfwSetKeyCallback(window, [](GLFWwindow *w, auto... args) {
    static_cast<Input *>(glfwGetWindowUserPointer(w))->handleKeyCallback(args...);
  });

  glfwSetScrollCallback(window, [](GLFWwindow *w, auto... args) {
    static_cast<Input *>(glfwGetWindowUserPointer(w))->handleScrollCallback(args...);
  });

  glfwSetCursorPosCallback(window, [](GLFWwindow *w, auto... args) {
    static_cast<Input *>(glfwGetWindowUserPointer(w))->handleCursorPosCallback(args...);
  });

  glfwSetMouseButtonCallback(window, [](GLFWwindow *w, auto... args) {
    static_cast<Input *>(glfwGetWindowUserPointer(w))->handleMouseButtonCallback(args...);
  });
}

void Input::update()
{
  g_engine->setCursorPos(currentCursorPos);

  for (const auto thunk : inputHooks)
  {
    thunk(this);
  }

  justPressedState.clear();
  this->currentKeyChange.key = -1;
  this->currentKeyChange.state = -1;

  this->currentScrollOffset.x = 0;
  this->currentScrollOffset.y = 0;

  this->currentLeftMouseEvent = -1;
  this->currentRightMouseEvent = -1;
}

void Input::registerHook(FunctionPtr ptr)
{
  inputHooks.emplace(ptr);
}

bool Input::justPressed(int key) const
{
  return getKeyState(key) == KeyState::JustPressed;
}

bool Input::pressed(int key) const
{
  return getKeyState(key) == KeyState::Pressed;
}

bool Input::pressed(const KeyCombination key) const
{
  bool pressed = getKeyState(key.key) == KeyState::Pressed;
  if (key.ctrl)
  {
    pressed = pressed && (getKeyState(GLFW_KEY_LEFT_CONTROL) != KeyState::Release || getKeyState(GLFW_KEY_RIGHT_CONTROL) != KeyState::Release);
  }
  if (key.shift)
  {
    pressed = pressed && (getKeyState(GLFW_KEY_LEFT_SHIFT) != KeyState::Release || getKeyState(GLFW_KEY_RIGHT_SHIFT) != KeyState::Release);
  }
  if (key.alt)
  {
    pressed = pressed && (getKeyState(GLFW_KEY_LEFT_ALT) != KeyState::Release || getKeyState(GLFW_KEY_RIGHT_ALT) != KeyState::Release);
  }

  return pressed;
}

KeyState Input::getKeyState(int key) const
{
  if (justPressedState.find(key) != justPressedState.end())
    return KeyState::JustPressed;

  switch (keyState.at(key))
  {
  case GLFW_RELEASE:
    return KeyState::Release;
  case GLFW_PRESS:
    // std::cout << value->second.prev << ", " << value->second.current << std::endl;
    return KeyState::Pressed;
  case GLFW_REPEAT:
    return KeyState::Repeat;
  default:
    DEBUG_ASSERT(false, "This should never happen.");
    return KeyState::Release;
  }
}

void Input::setKeyState(int key, int state)
{
  int prevState = keyState.at(key);
  keyState.at(key) = state;

  switch (state)
  {
  case GLFW_PRESS:
    pressedKeys.emplace(key);

    if (prevState == GLFW_RELEASE)
    {
      justPressedState.emplace(key);
    }
    break;
  case GLFW_RELEASE:
    pressedKeys.erase(key);
    break;
  case GLFW_REPEAT:
    if (!pressedKeys.empty())
    {
      for (const auto pressedKey : pressedKeys)
      {
        keyState.at(pressedKey) = GLFW_REPEAT;
      }
      pressedKeys.clear();
    }
  default:
    break;
  }
}

KeyCombination::KeyCombination(int key)
    : key(key), ctrl(false), shift(false), alt(false)
{
}

bool Input::released(int key) const
{
  return getKeyState(key) == KeyState::Release;
}

bool Input::down(int key) const
{
  return !released(key);
}

bool Input::repeated(int key) const
{
  return getKeyState(key) == KeyState::Repeat && glfwGetKey(window, key) == GLFW_PRESS;
}

bool Input::anyRepeated(std::initializer_list<int> keys) const
{
  return std::any_of(keys.begin(), keys.end(), [this](int key) { return repeated(key); });
}

bool Input::ctrl() const
{
  return getKeyState(GLFW_KEY_LEFT_CONTROL) != KeyState::Release || getKeyState(GLFW_KEY_RIGHT_CONTROL) != KeyState::Release;
}

bool Input::shift() const
{
  return getKeyState(GLFW_KEY_LEFT_SHIFT) != KeyState::Release || getKeyState(GLFW_KEY_RIGHT_SHIFT) != KeyState::Release;
}

bool Input::alt() const
{
  return getKeyState(GLFW_KEY_LEFT_ALT) != KeyState::Release || getKeyState(GLFW_KEY_RIGHT_ALT) != KeyState::Release;
}

int Input::keyEvent(GLFWKey key) const
{
  if (currentKeyChange.key == key)
    return currentKeyChange.state;
  else
  {
    return -1;
  }
}

bool Input::keyDownEvent(GLFWKey key) const
{
  return currentKeyChange.key == key && (currentKeyChange.state != GLFW_RELEASE);
}

std::pair<double, double> Input::scrollOfset() const
{
  return {currentScrollOffset.x, currentScrollOffset.y};
}

ScreenPosition Input::cursorPos() const
{
  return currentCursorPos;
}

bool Input::leftMouseDown() const
{
  int status = g_engine->captureMouse ? glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) : currentLeftMouseEvent;
  return status == GLFW_PRESS;
}
bool Input::rightMouseDown() const
{
  int status = g_engine->captureMouse ? glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) : currentRightMouseEvent;
  return status == GLFW_PRESS;
}

int Input::leftMouseEvent() const
{
  return currentLeftMouseEvent;
}

int Input::rightMouseEvent() const
{
  return currentLeftMouseEvent;
}

/**
 * 
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * Callback handlers
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 **/
void Input::handleScrollCallback(double xOffset, double yOffset)
{
  if (!g_engine->captureMouse)
    return;

  currentScrollOffset.x += xOffset;
  currentScrollOffset.y += yOffset;
}

void Input::handleKeyCallback(int key, int scancode, int action, int mods)
{
  if (!g_engine->captureKeyboard)
    return;

  currentKeyChange.key = key;
  currentKeyChange.state = action;

  if (key == GLFW_KEY_D && action == GLFW_PRESS)
  {
    g_engine->debug = !g_engine->debug;
  }
  if (!g_engine->captureKeyboard)
    return;

  setKeyState(key, action);
}

void Input::handleCursorPosCallback(double x, double y)
{
  if (!g_engine->captureMouse)
    return;

  currentCursorPos.x = x;
  currentCursorPos.y = y;
}

void Input::handleMouseButtonCallback(int button, int action, int mods)
{
  if (!g_engine->captureMouse)
    return;

  switch (button)
  {
  case GLFW_MOUSE_BUTTON_LEFT:
    currentLeftMouseEvent = action;
    break;
  case GLFW_MOUSE_BUTTON_RIGHT:
    currentRightMouseEvent = action;
    break;
  default:
    break;
  }
}
