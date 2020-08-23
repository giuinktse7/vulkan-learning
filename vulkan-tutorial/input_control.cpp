#include "input_control.h"

#include "graphics/engine.h"
#include <glm/glm.hpp>

#include "position.h"

void handleSelection(Input *input, Position &pos);
void handleBrush(Input *input, Position &pos);

void handleCameraZoom(Input *input);

void InputControl::mapEditing(Input *input)
{
  MapView &mapView = *g_engine->getMapView();

  if (input->keyEvent(GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    g_engine->clearBrush();
  }

  // Delete selected items
  if (input->keyDownEvent(GLFW_KEY_DELETE))
  {
    mapView.deleteSelectedItems();
  }

  // Undo the latest action
  if (input->ctrl() && input->keyDownEvent(GLFW_KEY_Z))
  {
    g_engine->getMapView()->undo();
  }

  Map *map = g_engine->getMapView()->getMap();

  auto selectedId = g_engine->getSelectedServerId();
  bool hasBrush = selectedId.has_value();

  Position pos = g_engine->getCursorPos().toPos(mapView);

  if (hasBrush)
  {
    handleBrush(input, pos);
  }
  else
  {
    handleSelection(input, pos);
  }
}

void handleBrush(Input *input, Position &pos)
{
  MapView &mapView = *g_engine->getMapView();
  auto selectedId = g_engine->getSelectedServerId();

  if (input->leftMouseEvent() == GLFW_PRESS) // Mouse press event
  {
    Logger::debug("handleBrush GLFW_PRESS");
    if (!mapView.history.currentGroupType(ActionGroupType::AddMapItem))
    {
      mapView.history.startGroup(ActionGroupType::AddMapItem);
    }
    mapView.addItem(pos, selectedId.value());
  }
  else if (input->leftMouseDown()) // Mouse down event (fast)
  {
    MapPosition oldMapPos = g_engine->getPrevCursorPos().worldPos(mapView).mapPos();
    MapPosition mapPos = g_engine->getCursorPos().worldPos(mapView).mapPos();
    if (mapPos != oldMapPos)
    {
      if (!mapView.history.hasCurrentGroup())
      {
        mapView.history.startGroup(ActionGroupType::AddMapItem);
      }
      mapView.addItem(pos, selectedId.value());
    }
  }
  else if (input->leftMouseEvent() == GLFW_RELEASE) // Mouse release event
  {
    Logger::debug("handleBrush GLFW_RELEASE");

    if (mapView.history.hasCurrentGroup())
    {
      mapView.history.endGroup(ActionGroupType::AddMapItem);
    }
  }
}

void handleSelection(Input *input, Position &pos)
{
  MapView &mapView = *g_engine->getMapView();
  Map *map = mapView.getMap();

  if (input->leftMouseEvent() == GLFW_PRESS)
  {
    bool startDragAction = mapView.isEmpty(pos) || input->shift();
    if (startDragAction)
    {
      if (!input->ctrl())
      {
        mapView.clearSelection();
      }

      mapView.setDragStart(input->getCursorPos().worldPos(mapView));
    }
    else // Do not start a drag action
    {
      Tile *tile = map->getTile(pos);
      if (tile->hasTopItem())
      {
        Item &topItem = *tile->getTopItem();
        if (tile->topItemSelected())
        {
          mapView.selection.moveOrigin = pos;
        }
        else // The top item is not selected
        {
          if (!input->ctrl())
          {
            mapView.clearSelection();
          }
          mapView.selection.blockDeselect = true;

          if (input->shift())
          {
            mapView.selectAll(*tile);
          }
          else // Shift is not down
          {
            mapView.history.startGroup(ActionGroupType::Selection);
            mapView.selectTopItem(*tile);
            mapView.history.endGroup(ActionGroupType::Selection);
          }
        }
      }
    }
  }
  else if (input->leftMouseDown()) // Mouse down event (fast)
  {
    if (mapView.isDragging())
    {
      mapView.setDragEnd(input->getCursorPos().worldPos(mapView));
    }

    if (mapView.hasSelectionMoveOrigin() && !mapView.isSelectionMoving())
    {
      if (mapView.selection.moveOrigin.value() != pos)
      {
        mapView.selection.moving = true;
      }
    }
  }
  else if (input->leftMouseEvent() == GLFW_RELEASE) // Mouse release event
  {
    if (mapView.isDragging())
    {
      mapView.endDragging();
    }

    mapView.finishMoveSelection(pos);

    if (mapView.selection.blockDeselect)
    {
      mapView.selection.blockDeselect = false;
    }
    else // Deselect not blocked
    {
      Tile *tile = map->getTile(pos);
      if (tile && tile->topItemSelected())
      {
        mapView.history.startGroup(ActionGroupType::Selection);
        mapView.deselectTopItem(*tile);
        mapView.history.endGroup(ActionGroupType::Selection);
      }
    }
  }
}

bool InputControl::cursorChangedMapTile()
{
  MapView &mapView = *g_engine->getMapView();

  MapPosition oldMapPos = g_engine->getPrevCursorPos().worldPos(mapView).mapPos();
  MapPosition mapPos = g_engine->getCursorPos().worldPos(mapView).mapPos();

  return oldMapPos != mapPos;
}

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