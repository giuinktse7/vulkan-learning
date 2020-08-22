#include "selection.h"

bool Selection::contains(Tile *tile) const
{
  return selectedTiles.find(tile) != selectedTiles.end();
}

void Selection::select(Tile *tile)
{
  selectedTiles.emplace(tile);
}

void Selection::deselect(Tile *tile)
{
  selectedTiles.erase(tile);
}

void Selection::addTile(Tile *tile)
{
  select(tile);
}
void Selection::removeTile(Tile *tile)
{
  deselect(tile);
}

std::unordered_set<Tile *, TileHash, TileEquality> Selection::getTiles() const
{
  return selectedTiles;
}

void Selection::clear()
{
  selectedTiles.clear();
}

void Selection::deselectAll()
{
  for (Tile *tile : selectedTiles)
  {
    tile->deselect();
  }

  selectedTiles.clear();
}

bool Selection::empty() const
{
  return selectedTiles.empty();
}
