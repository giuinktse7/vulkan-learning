#include "position.h"

#include "map_view.h"

WorldPosition ScreenPosition::worldPos(const MapView &mapView) const
{
  double newX = mapView.getX() + this->x / mapView.getZoomFactor();
  double newY = mapView.getY() + this->y / mapView.getZoomFactor();

  return WorldPosition{newX, newY};
}

MapPosition ScreenPosition::mapPos(const MapView &mapView) const
{
  double newX = this->x / mapView.getZoomFactor();
  double newY = this->y / mapView.getZoomFactor();

  newX = std::floor(newX / MapTileSize);
  newY = std::floor(newY / MapTileSize);

  return MapPosition{static_cast<long>(newX), static_cast<long>(newY)};
}

MapPosition WorldPosition::mapPos() const
{
  return MapPosition{
      static_cast<long>(std::floor(this->x / MapTileSize)),
      static_cast<long>(std::floor(this->y / MapTileSize))};
}

WorldPosition MapPosition::worldPos() const
{
  return WorldPosition{static_cast<double>(this->x * MapTileSize), static_cast<double>(this->y * MapTileSize)};
}

Position MapPosition::floor(int floor) const
{
  Position pos;
  pos.x = this->x;
  pos.y = this->y;
  pos.z = floor;

  return pos;
}

Position ScreenPosition::toPos(const MapView &mapView) const
{
  return worldPos(mapView).mapPos().floor(mapView.getFloor());
}

Position WorldPosition::toPos(const MapView &mapView) const
{
  return toPos(mapView.getFloor());
}

Position WorldPosition::toPos(int floor) const
{
  return mapPos().floor(floor);
}
