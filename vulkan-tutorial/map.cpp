#include "map.h"

#include "graphics/engine.h"

#include <iostream>

#include "tile_location.h"
#include "debug.h"

#include <stack>

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

Map::Iterator *Map::Iterator::nextFromLeaf()
{
  auto &node = stack.top().node;
  DEBUG_ASSERT(node.isLeaf(), "The node must be a leaf node.");

  for (uint32_t z = this->floorIndex; z < MAP_LAYERS; ++z)
  {
    if (Floor *floor = node.getFloor(z))
    {
      for (uint32_t i = this->tileIndex; i < MAP_LAYERS; ++i)
      {
        TileLocation &location = floor->getTileLocation(i);
        if (location.hasTile())
        {
          this->value = &location;
          this->floorIndex = z;
          this->tileIndex = i + 1;

          return this;
        }
      }
    }
  }

  return nullptr;
}

Map::Iterator Map::begin()
{
  Map::Iterator iterator;
  iterator.push(this->root);

  while (!iterator.stack.empty())
  {
    auto current = iterator.stack.top();

    if (current.node.isLeaf())
    {
      auto next = iterator.nextFromLeaf();
      return next ? *next : end();
    }

    uint32_t size = current.node.nodes.size();
    for (uint32_t i = current.cursor; i < size; ++i)
    {
      if (auto &child = current.node.nodes[i])
      {
        current.cursor = i + 1;
        iterator.push(*child.get());
        break;
      }
    }

    if (&iterator.stack.top().node == &current.node)
    {
      return end();
    }
  }

  return end();
}

void Map::Iterator::finish()
{
  this->value = nullptr;
  this->tileIndex = 0;
  this->floorIndex = 0;
}

Map::Iterator &Map::Iterator::operator++()
{
  while (!stack.empty())
  {
    auto &current = stack.top();

    if (current.node.isLeaf())
    {
      auto next = nextFromLeaf();
      if (!next)
      {
        stack.pop();
        continue;
      }

      return *next;
    }

    tileIndex = 0;
    floorIndex = 0;

    uint32_t size = current.node.nodes.size();
    // This node is finished
    if (current.cursor == size)
    {
      stack.pop();
      continue;
    }

    for (; current.cursor < size; ++current.cursor)
    {
      if (auto &child = current.node.nodes[current.cursor])
      {
        current.cursor += 1;
        push(*child.get());
        break;
      }
    }
  }

  this->finish();
  return *this;
}

Map::Iterator Map::end()
{
  return Map::Iterator{};
}

Map::Iterator::Iterator()
{
  // std::cout << "MapIterator()" << std::endl;
}

Map::Iterator::~Iterator()
{
  // std::cout << "~MapIterator()" << std::endl;
}

void Map::printTiles()
{
  std::stack<quadtree::Node *> stack;
  stack.push(&this->root);

  while (!stack.empty())
  {
    auto current = stack.top();
    stack.pop();

    if (current->isLeaf())
    {
      for (uint32_t z = 0; z < MAP_LAYERS; ++z)
      {
        Floor *floor = current->getFloor(z);
        if (floor)
        {
          for (uint32_t i = 0; i < MAP_LAYERS; ++i)
          {
            TileLocation &location = floor->getTileLocation(i);
            if (location.getTile())
            {
              std::cout << location.getPosition() << std::endl;
            }
          }
        }
      }
    }
    else
    {
      for (uint32_t i = 0; i < MAP_LAYERS; ++i)
      {
        auto &node = current->nodes[MAP_LAYERS - i - 1];
        if (node)
        {
          stack.push(node.get());
        }
      }
    }
  }
}

TileLocation *Map::Iterator::operator*()
{
  return value;
}

TileLocation *Map::Iterator::operator->()
{
  return value;
}
