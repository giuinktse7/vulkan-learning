#include "tile_location.h"

#include <memory>
#include <iostream>
#include "tile.h"

TileLocation::TileLocation()
{
  // std::cout << "TileLocation()" << std::endl;
}

TileLocation::~TileLocation()
{
  // std::cout << "~TileLocation()" << std::endl;
}

void TileLocation::setTile(std::unique_ptr<Tile> tile)
{
  this->tile = std::move(tile);
}
Tile *TileLocation::getTile() const
{
  return tile ? tile.get() : nullptr;
}

const bool TileLocation::hasTile() const
{
  return getTile() != nullptr;
}

void TileLocation::removeTile()
{
  this->tile = nullptr;
}

std::unique_ptr<Tile> TileLocation::replaceTile(Tile &&newTile)
{
  std::unique_ptr<Tile> old = std::move(this->tile);
  this->tile = std::make_unique<Tile>(std::move(newTile));
  return old;
}