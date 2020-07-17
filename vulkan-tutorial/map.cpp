#include "map.h"

#include "tile_location.h"
#include "debug.h"

Map::Map()
    : root(NodeType::Root)
{
}

Tile &Map::createTile(int x, int y, int z)
{
  DEBUG_ASSERT(root.isRoot(), "Only root nodes can create a tile.");
  auto &leaf = root.getLeafWithCreate(x, y);

  DEBUG_ASSERT(leaf.isLeaf(), "The node must be a leaf node.");

  Floor &floor = leaf.createFloor(x, y, z);
  TileLocation &location = floor.getTileLocation(x, y);

  if (!location.get())
  {
    location.setTile(std::move(std::make_unique<Tile>(location)));
  }

  return *location.get();
}
