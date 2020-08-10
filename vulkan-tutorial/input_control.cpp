#include "input_control.h"

#include "graphics/engine.h"
#include <glm/glm.hpp>

#include "position.h"

#include "ecs/item_selection.h"

void handleCameraZoom(Input *input)
{
  const auto [x, y] = input->scrollOfset();
  if (y > 0)
  {
    g_engine->zoomIn();
  }
  else if (y < 0)
  {
    g_engine->zoomOut();
  }

  if (input->keyDownEvent(GLFW_KEY_0) && input->ctrl())
  {
    g_engine->resetZoom();
  }
}

void handleSelectionOnClick(Input *input, Position &pos)
{
  Map *map = g_engine->getMapView()->getMap();
  Tile *tile = map->getTile(pos);

  if (!input->ctrl())
  {
    g_ecs.getSystem<TileSelectionSystem>().clearAllSelections();
  }

  if (tile == nullptr)
  {
    return;
  }

  if (!tile->entity.has_value())
  {
    Entity entity = g_ecs.createEntity();
    tile->entity = entity;
  }
  Entity entity = tile->entity.value();

  auto *selection = g_ecs.getComponent<TileSelectionComponent>(entity);
  if (selection == nullptr)
  {
    TileSelectionComponent component;
    component.position = pos;
    g_ecs.addComponent(entity, component);
  }
  selection = g_ecs.getComponent<TileSelectionComponent>(entity);

  if (input->shift())
  {
    selection->toggleSelectAll();
  }
  else
  {
    auto topItem = tile->getTopItem();
    if (topItem != nullptr)
    {
      if (topItem == tile->getGround())
      {
        selection->toggleSelection(TileEntity::Ground);
      }
      else
      {
        selection->toggleItemSelection(tile->getItemCount() - 1);
      }
    }
  }
}

void InputControl::cameraMovement(Input *input)
{
  handleCameraZoom(input);

  glm::vec3 delta{};
  float step = Engine::TILE_SIZE / (std::pow(g_engine->getMapView()->getZoomFactor(), 1.5f));

  bool anyRepeated = input->anyRepeated({GLFW_KEY_RIGHT, GLFW_KEY_LEFT, GLFW_KEY_UP, GLFW_KEY_DOWN});

  if (anyRepeated) // Smooth panning
  {
    if (input->down(GLFW_KEY_RIGHT))
    {
      delta.x += step;
    }
    if (input->down(GLFW_KEY_LEFT))
    {
      delta.x -= step;
    }
    if (input->down(GLFW_KEY_UP))
    {
      delta.y -= step;
    }
    if (input->down(GLFW_KEY_DOWN))
    {
      delta.y += step;
    }
  }
  else // Normal panning
  {

    if (input->repeated(GLFW_KEY_RIGHT) || input->keyEvent(GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
      delta.x += step;
    }
    if (input->repeated(GLFW_KEY_LEFT) || input->keyEvent(GLFW_KEY_LEFT) == GLFW_PRESS)
    {
      delta.x -= step;
    }
    if (input->repeated(GLFW_KEY_UP) || input->keyEvent(GLFW_KEY_UP) == GLFW_PRESS)
    {
      delta.y -= step;
    }
    if (input->repeated(GLFW_KEY_DOWN) || input->keyEvent(GLFW_KEY_DOWN) == GLFW_PRESS)
    {
      delta.y += step;
    }
  }

  if (input->keyEvent(GLFW_KEY_KP_ADD) == GLFW_PRESS || input->keyEvent(GLFW_KEY_KP_ADD) == GLFW_REPEAT)
  {
    delta.z -= 1;
  }
  if (input->keyEvent(GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || input->keyEvent(GLFW_KEY_KP_SUBTRACT) == GLFW_REPEAT)
  {
    delta.z += 1;
  }

  if (delta.x != 0 || delta.y != 0 || delta.z != 0)
  {
    g_engine->getMapView()->translateCamera(delta);
  }
}

void InputControl::mapEditing(Input *input)
{
  if (input->keyEvent(GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    g_engine->clearBrush();
  }

  MapView &mapView = *g_engine->getMapView();
  Map *map = g_engine->getMapView()->getMap();

  MapPosition oldMapPos = g_engine->getPrevCursorPos().worldPos(mapView).mapPos();
  MapPosition mapPos = g_engine->getCursorPos().worldPos(mapView).mapPos();

  auto selectedId = g_engine->getSelectedServerId();

  Position pos = mapPos.floor(mapView.getFloor());

  if (selectedId.has_value())
  {
    if (oldMapPos != mapPos)
    {
      // Dragging mouse
      if (input->leftMouseDown())
      {
        map->createItemAt(pos, selectedId.value());
      }
    }
    else if (input->leftMouseEvent() == GLFW_PRESS)
    {
      map->createItemAt(pos, selectedId.value());
    }
  }
  else
  {
    if (input->leftMouseEvent() == GLFW_PRESS)
    {
      handleSelectionOnClick(input, pos);
    }
  }
}
