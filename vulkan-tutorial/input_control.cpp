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

  if (tile == nullptr)
  {
    if (!input->ctrl())
    {
      g_ecs.getSystem<TileSelectionSystem>().clearAllSelections();
    }

    return;
  }

  MapView &mapView = *g_engine->getMapView();
  ecs::EntityId entityId = tile->getOrCreateEntity();

  auto *selection = tile->getComponent<TileSelectionComponent>();
  if (selection)
  {
    // Move selection
    if (selection->isTopSelected() && InputControl::cursorChangedMapTile())
    {
      Position pos = g_engine->getCursorPos().toPos(mapView);
      std::cout << "moveSelectionOrigin: " << pos << std::endl;
      mapView.moveSelectionOrigin = pos;
      // Rest of function is selection logic unrelated to dragging
      return;
    }
    if (!input->ctrl())
    {
      g_ecs.getSystem<TileSelectionSystem>().clearAllSelections();
    }
  }
  else
  {
    if (!input->ctrl())
    {
      g_ecs.getSystem<TileSelectionSystem>().clearAllSelections();
    }

    TileSelectionComponent component{};
    component.position = pos;
    component.tileItemCount = tile->getItemCount();

    selection = tile->addComponent(component);
  }

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

  if (selection && InputControl::cursorChangedMapTile())
  {
    // Move selection
    if (selection->isTopSelected())
    {
      Position pos = g_engine->getCursorPos().toPos(mapView);
      mapView.moveSelectionOrigin = pos;
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

bool InputControl::cursorChangedMapTile()
{
  MapView &mapView = *g_engine->getMapView();

  MapPosition oldMapPos = g_engine->getPrevCursorPos().worldPos(mapView).mapPos();
  MapPosition mapPos = g_engine->getCursorPos().worldPos(mapView).mapPos();

  return oldMapPos != mapPos;
}

void InputControl::mapEditing(Input *input)
{
  MapView &mapView = *g_engine->getMapView();

  if (input->keyEvent(GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    g_engine->clearBrush();
  }

  if (input->keyDownEvent(GLFW_KEY_DELETE))
  {
    g_ecs.getSystem<TileSelectionSystem>().deleteItems();
  }

  if (input->ctrl() && input->keyDownEvent(GLFW_KEY_Z))
  {
    g_engine->getMapView()->undo();
  }

  if (input->leftMouseEvent() == GLFW_RELEASE)
  {
    Logger::debug("input->leftMouseEvent() == GLFW_RELEASE");
    mapView.moveSelectionOrigin.reset();
  }

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
        if (!mapView.history.hasCurrentGroup())
        {
          mapView.history.startGroup(ActionGroupType::AddMapItem);
        }
        mapView.addItem(pos, selectedId.value());
      }
    }
    else if (input->leftMouseEvent() == GLFW_PRESS)
    {
      mapView.history.startGroup(ActionGroupType::AddMapItem);
      mapView.addItem(pos, selectedId.value());
    }

    if (input->leftMouseEvent() == GLFW_RELEASE)
    {
      if (mapView.history.hasCurrentGroup())
      {
        mapView.history.endGroup(ActionGroupType::AddMapItem);
      }
    }
  }
  else
  {
    if (input->leftMouseEvent() == GLFW_PRESS)
    {
      std::cout << "input->leftMouseEvent() == GLFW_PRESS" << std::endl;
      handleSelectionOnClick(input, pos);
    }
  }
}