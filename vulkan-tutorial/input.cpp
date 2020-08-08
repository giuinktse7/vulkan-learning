#include "input.h"

#include "graphics/engine.h"
#include "ecs/ecs.h"
#include "ecs/item_selection.h"

void updateKeyState(int key, int action)
{
  g_engine->setKeyState(key, action);
}

bool fastKeyDown(GLFWwindow *window, int key)
{
  return g_engine->getKeyState(key) == GLFW_REPEAT && glfwGetKey(window, key) != GLFW_RELEASE;
}

void Input::update(GLFWwindow *window)
{
  glm::vec3 delta{};
  float step = Engine::TILE_SIZE / (std::pow(g_engine->getMapRenderer()->camera.zoomFactor, 1.5));

  if (fastKeyDown(window, GLFW_KEY_RIGHT))
  {
    delta.x += step;
  }
  if (fastKeyDown(window, GLFW_KEY_LEFT))
  {
    delta.x -= step;
  }
  if (fastKeyDown(window, GLFW_KEY_UP))
  {
    delta.y -= step;
  }
  if (fastKeyDown(window, GLFW_KEY_DOWN))
  {
    delta.y += step;
  }

  if (delta.x != 0 || delta.y != 0 || delta.z != 0)
  {
    g_engine->getMapRenderer()->camera.translate(delta);
  }
}

void updateCamera(int key)
{

  if (key == GLFW_KEY_KP_ADD)
  {
    g_engine->translateCameraZ(-1);
  }
  if (key == GLFW_KEY_KP_SUBTRACT)
  {
    g_engine->translateCameraZ(+1);
  }

  glm::vec3 delta(0.0f, 0.0f, 0.0f);
  float step = Engine::TILE_SIZE / (std::pow(g_engine->getMapRenderer()->camera.zoomFactor, 1.5));

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
  case GLFW_KEY_KP_ADD:

  default:
    break;
  }

  if (delta.x != 0 || delta.y != 0)
  {
    g_engine->translateCamera(delta);
  }
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
  if (key == GLFW_KEY_D && action == GLFW_PRESS)
  {
    g_engine->debug = !g_engine->debug;
  }
  if (!g_engine->captureKeyboard)
    return;

  // if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
  // {
  updateKeyState(key, action);
  // }

  if (key == GLFW_KEY_0)
  {
    bool ctrlDown = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) != GLFW_RELEASE || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) != GLFW_RELEASE;
    if (ctrlDown)
    {
      g_engine->resetZoom();
      std::cout << "Reset zoom" << std::endl;
    }
  }

  bool keyActive = action == GLFW_PRESS || action == GLFW_REPEAT;

  if (!keyActive)
  {
    return;
  }

  if (key == GLFW_KEY_ESCAPE)
  {
    g_engine->clearBrush();
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

  // Create items when dragging mouse
  if (g_engine->getSelectedServerId().has_value() && oldGamePos != newGamePos)
  {
    // Dragging mouse
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
      uint16_t selectedId = g_engine->getSelectedServerId().value();

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
    bool activeBrush = g_engine->getSelectedServerId().has_value();

    auto &map = *g_engine->getMapRenderer()->map;
    auto pos = g_engine->screenToGamePos(g_engine->getMousePosition());

    if (activeBrush)
    {
      uint16_t selectedId = g_engine->getSelectedServerId().value();

      map.createItemAt(pos, selectedId);
    }
    else
    {
      Tile *tile = map.getTile(pos);
      if (tile != nullptr)
      {
        if (!tile->entity.has_value())
        {
          Entity entity = g_ecs.createEntity();
          tile->entity = entity;
        }

        Entity entity = tile->entity.value();
        TileSelectionComponent *selection = g_ecs.getComponent<TileSelectionComponent>(entity);
        if (selection == nullptr)
        {
          TileSelectionComponent component;
          component.position = pos;
          g_ecs.addComponent(entity, component);
        }

        selection = g_ecs.getComponent<TileSelectionComponent>(entity);
        std::cout << selection->position << std::endl;

        auto topItem = tile->getTopItem();
        if (topItem != nullptr)
        {
          if (topItem == tile->getGround())
          {
            selection->toggleSelection(TileEntity::Ground);
          }
          else
          {
            selection->selectItemIndex(tile->getItemCount() - 1);
          }
        }
      }
    }
  }
}
