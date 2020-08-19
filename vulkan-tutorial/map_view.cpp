#include "map_view.h"

#include "const.h"

MapView::MapView(GLFWwindow *window)
    : window(window),
      map(std::make_shared<Map>()),
      moveSelectionOrigin{},
      dragState{}
{
}

void MapView::addItem(const Position pos, uint16_t id)
{
  Item item(id);

  const SpriteInfo &spriteInfo = item.itemType->appearance->getSpriteInfo();
  if (spriteInfo.hasAnimation())
  {
    auto &anim = *spriteInfo.getAnimation();
    ecs::EntityId entityId = item.assignNewEntityId();
    g_ecs.addComponent(entityId, ItemAnimationComponent(spriteInfo.getAnimation()));
  }

  Tile &currentTile = map->getOrCreateTile(pos);
  Tile newTile = currentTile.deepCopy();
  newTile.addItem(std::move(item));

  MapAction action(*this, MapActionType::CreateTiles);
  action.addChange(std::move(newTile));
  history.commit(std::move(action));
}

void MapView::removeItems(const Position position, const std::set<size_t, std::greater<size_t>> &indices)
{
  TileLocation *location = map->getTileLocation(position);

  DEBUG_ASSERT(location->hasTile(), "The location has no tile.");
  Tile *tile = location->getTile();

  MapAction action(*this, MapActionType::DeleteTiles);

  Tile newTile = deepCopyTile(position);

  for (const auto index : indices)
  {
    newTile.removeItem(index);
  }

  action.addChange(std::move(newTile));

  history.commit(std::move(action));
}

void MapView::removeTile(const Position position)
{
  MapAction action(*this, MapActionType::DeleteTiles);

  action.addChange(Change::removeTile(position));

  history.commit(std::move(action));
}

void MapView::removeGround(Position position)
{
  TileLocation *location = map->getTileLocation(position);

  DEBUG_ASSERT(location->hasTile(), "The location has no tile.");
  Tile *tile = location->getTile();

  DEBUG_ASSERT(tile->getGround() != nullptr, "There is no ground to remove.");

  MapAction action(*this, MapActionType::DeleteTiles);

  Tile newTile = deepCopyTile(position);
  newTile.removeGround();
  action.addChange(std::move(newTile));

  history.commit(std::move(action));
}

void MapView::updateViewport()
{
  glfwGetFramebufferSize(window, &viewport.width, &viewport.height);

  viewport.zoom = 1 / camera.zoomFactor;
  viewport.offsetX = camera.position.x;
  viewport.offsetY = camera.position.y;
}

void MapView::zoomOut()
{
  camera.zoomOut();
}
void MapView::zoomIn()
{
  camera.zoomIn();
}
void MapView::resetZoom()
{
  camera.resetZoom();
}

float MapView::getZoomFactor() const
{
  return camera.zoomFactor;
}

void MapView::translateCamera(glm::vec3 delta)
{
  camera.translate(delta);
}
void MapView::translateCameraZ(int z)
{
  camera.translateZ(z);
}

util::Rectangle<int> MapView::getGameBoundingRect() const
{
  WorldPosition worldPos{static_cast<double>(viewport.offsetX), static_cast<double>(viewport.offsetY)};
  MapPosition mapPos = worldPos.mapPos();

  auto [width, height] = ScreenPosition{static_cast<double>(viewport.width), static_cast<double>(viewport.height)}.mapPos(*this);
  util::Rectangle<int> rect;
  rect.x1 = mapPos.x;
  rect.y1 = mapPos.y;
  rect.x2 = mapPos.x + width;
  rect.y2 = mapPos.y + height;

  return rect;
}

void MapView::setDragStart(WorldPosition position)
{
  if (dragState.has_value())
  {
    dragState.value().from = position;
  }
  else
  {
    dragState = {position, position};
  }
}

bool MapView::isEmpty(Position position) const
{
  return map->isTileEmpty(position);
}

void MapView::setDragEnd(WorldPosition position)
{
  DEBUG_ASSERT(dragState.has_value(), "There is no current dragging operation.");

  dragState.value().to = position;
}

void MapView::endDragging()
{
  dragState.reset();
}

bool MapView::isDragging() const
{
  return dragState.has_value();
}