#include "input.h"

#include "graphics/engine.h"

void updateModifierKey(int key, int action)
{
  if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
  {
    if (action == GLFW_RELEASE)
    {
      g_engine->setKeyUp(key);
    }
    else if (action == GLFW_PRESS)
    {
      g_engine->setKeyDown(key);
    }
  }
}

void updateCamera(int key)
{
  bool ctrlDown = g_engine->isCtrlDown();

  if (ctrlDown && key == GLFW_KEY_0)
  {
    g_engine->resetZoom();
  }

  glm::vec2 delta(0.0f, 0.0f);
  float step = Engine::TILE_SIZE / std::pow(g_engine->getMapRenderer()->camera.zoomFactor, 2);

  switch (key)
  {
  case GLFW_KEY_RIGHT:
    delta.x = step;
    break;
  case GLFW_KEY_LEFT:
    delta.x = -step;
    break;
  case GLFW_KEY_UP:
    delta.y = -step;
    break;
  case GLFW_KEY_DOWN:
    delta.y = step;
    break;
  default:
    break;
  }

  g_engine->translateCamera(delta);
}

void Input::handleMouseScroll(GLFWwindow *window, double xoffset, double yoffset)
{
  if (!g_engine->captureMouse)
    return;

  if (yoffset > 0)
  {
    g_engine->zoomIn();
  }
  else
  {
    g_engine->zoomOut();
  }
}

void Input::handleKeyAction(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (!g_engine->captureKeyboard)
    return;

  if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
  {
    updateModifierKey(key, action);
  }

  bool keyActive = action == GLFW_PRESS || action == GLFW_REPEAT;

  if (!keyActive)
  {
    return;
  }

  updateCamera(key);
}

void Input::handleCursorPosition(GLFWwindow *window, double x, double y)
{
  if (!g_engine->captureMouse)
    return;

  Position oldGamePos = g_engine->screenToGamePos(g_engine->getMousePosition());
  g_engine->setMousePosition(static_cast<float>(x), static_cast<float>(y));
  Position newGamePos = g_engine->screenToGamePos(g_engine->getMousePosition());

  if (oldGamePos != newGamePos)
  {
    // Dragging mouse
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
      uint16_t selectedId = g_engine->getSelectedServerId();

      auto &map = *g_engine->getMapRenderer()->map;
      auto pos = g_engine->screenToGamePos(g_engine->getMousePosition());

      map.createItemAt(pos, selectedId);
    }
  }
}

void Input::handleMouseKeyAction(GLFWwindow *window, int button, int action, int mods)
{
  if (!g_engine->captureMouse)
    return;

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
  {
    uint16_t selectedId = g_engine->getSelectedServerId();

    auto &map = *g_engine->getMapRenderer()->map;
    auto pos = g_engine->screenToGamePos(g_engine->getMousePosition());

    map.createItemAt(pos, selectedId);
  }
}
