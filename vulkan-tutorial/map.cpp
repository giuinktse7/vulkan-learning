#include "map.h"

#include "graphics/engine.h"

#include <iostream>

#include "tile_location.h"
#include "debug.h"

Map::Map()
    : root(NodeType::Root)
{
}

Tile &Map::getOrCreateTile(Position &pos)
{
  return getOrCreateTile(pos.x, pos.y, pos.z);
}

Tile &Map::getOrCreateTile(int x, int y, int z)
{
  DEBUG_ASSERT(root.isRoot(), "Only root nodes can create a tile.");
  auto &leaf = root.getLeafWithCreate(x, y);

  DEBUG_ASSERT(leaf.isLeaf(), "The node must be a leaf node.");

  Floor &floor = leaf.getOrCreateFloor(x, y, z);
  TileLocation &location = floor.getTileLocation(x, y);

  if (!location.getTile())
  {
    location.setTile(std::move(std::make_unique<Tile>(location)));
  }

  return *location.getTile();
}

TileLocation *Map::getTileLocation(int x, int y, int z) const
{
  DEBUG_ASSERT(z >= 0 && z < MAP_LAYERS, "Z value '" + std::to_string(z) + "' is out of bounds.");
  quadtree::Node *leaf = root.getLeafUnsafe(x, y);
  if (leaf)
  {
    Floor *floor = leaf->getFloor(z);
    return &floor->getTileLocation(x, y);
  }

  return nullptr;
}

MapIterator Map::begin()
{
  MapIterator iterator;
  iterator.pushNode(&root);

  while (true)
  {
    auto &current = iterator.stack.back();
    quadtree::Node *node = current.node;
    int &index = current.index;

    bool unwind = false;
    for (; index < MAP_TREE_CHILDREN_COUNT; ++index)
    {
      auto &child = node->nodes[index];
      if (child)
      {
        if (child->isLeaf())
        {
          for (iterator.localZ = 0; iterator.localZ < MAP_LAYERS; ++iterator.localZ)
          {
            Floor *floor = child->getFloor(iterator.localZ);
            if (floor)
            {
              for (iterator.localI = 0; iterator.localI < MAP_LAYERS; ++iterator.localI)
              {
                TileLocation &location = floor->getTileLocation(iterator.localI);
                if (location.getTile())
                {
                  iterator.currentTile = &location;
                  return iterator;
                }
              }
            }
          }
        }
        else
        {
          ++index;
          iterator.pushNode(child.get());
          unwind = true;
          break;
        }
      }
    }

    if (unwind)
      continue;

    iterator.stack.pop_back();
    if (iterator.stack.empty())
      break;
  }

  return end();
}

MapIterator Map::end()
{
  MapIterator iterator;
  iterator.localI = -1;
  iterator.localZ = -1;
  return iterator;
}

MapIterator &MapIterator::operator++()
{
  bool increased = false;
  bool first = true;

  while (true)
  {
    MapIterator::NodeIndex &current = stack.back();
    quadtree::Node *node = current.node;
    int &index = current.index;

    bool unwind = false;

    for (; index < MAP_TREE_CHILDREN_COUNT; ++index)
    {
      auto &child = node->nodes[index];
      if (child)
      {
        if (child->isLeaf())
        {
          for (; localZ < MAP_LAYERS; ++localZ)
          {
            Floor *floor = child->getFloor(localZ);
            if (floor)
            {
              for (; localI < MAP_LAYERS; ++localI)
              {
                TileLocation &location = floor->getTileLocation(localI);
                if (location.getTile())
                {
                  if (increased)
                  {
                    currentTile = &location;
                    return *this;
                  }
                  else
                  {
                    increased = true;
                  }
                }
                else if (first)
                {
                  increased = true;
                  first = false;
                }
              }

              if (localI == MAP_TREE_CHILDREN_COUNT)
              {
                localI = 0;
              }
            }
          }

          if (localZ == MAP_LAYERS)
          {
            localZ = 0;
          }
        }
        // Not a leaf
        else
        {
          ++index;
          this->pushNode(child.get());
        }
      }
    }

    if (unwind)
      continue;

    stack.pop_back();
    if (stack.size() == 0)
    {
      localZ = -1;
      localI = -1;
      currentTile = nullptr;
      return *this;
    }
  }

  return *this;
}

MapIterator::MapIterator()
{
  // std::cout << "MapIterator()" << std::endl;
}

MapIterator::~MapIterator()
{
  // std::cout << "~MapIterator()" << std::endl;
}

TileLocation *MapIterator::operator*()
{
  return currentTile;
}

TileLocation *MapIterator::operator->()
{
  return currentTile;
}
