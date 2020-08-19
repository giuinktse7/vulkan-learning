#include "map.h"

#include "graphics/engine.h"

#include <iostream>

#include "tile_location.h"
#include "ecs/ecs.h"
#include "ecs/item_animation.h"
#include "debug.h"
#include "graphics/appearances.h"

#include <stack>

Map::Map()
    : root(quadtree::Node::NodeType::Root), width(2048), height(2048)
{
}

Tile &Map::getOrCreateTile(const Position &pos)
{
  return getOrCreateTile(pos.x, pos.y, pos.z);
}

std::unique_ptr<Tile> Map::replaceTile(Tile &&tile)
{
  auto pos = tile.getPosition();
  auto &leaf = root.getLeafWithCreate(pos.x, pos.y);
  TileLocation &location = leaf.getOrCreateTileLocation(tile.getPosition());

  return location.replaceTile(std::move(tile));
}

void Map::removeTile(const Position pos)
{
  auto leaf = root.getLeafUnsafe(pos.x, pos.y);
  if (leaf)
  {
    Floor *floor = leaf->getFloor(pos.z);
    if (floor)
    {
      auto &loc = floor->getTileLocation(pos.x, pos.y);
      if (loc.hasTile())
      {
        loc.removeTile();
      }
    }
  }
}

bool Map::isTileEmpty(const Position pos) const
{
  Tile *tile = getTile(pos);
  return !tile || tile->isEmpty();
}

Tile *Map::getTile(const Position pos) const
{
  auto leaf = root.getLeafUnsafe(pos.x, pos.y);
  if (!leaf)
    return nullptr;

  Floor *floor = leaf->getFloor(pos.z);
  if (!floor)
    return nullptr;

  return floor->getTileLocation(pos.x, pos.y).getTile();
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

TileLocation *Map::getTileLocation(const Position &pos) const
{
  return getTileLocation(pos.x, pos.y, pos.z);
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

quadtree::Node *Map::getLeafUnsafe(int x, int y)
{
  return root.getLeafUnsafe(x, y);
}

MapIterator *MapIterator::nextFromLeaf()
{
  quadtree::Node *node = stack.top().node;
  DEBUG_ASSERT(node->isLeaf(), "The node must be a leaf node.");

  for (int z = this->floorIndex; z < MAP_LAYERS; ++z)
  {
    if (Floor *floor = node->getFloor(z))
    {
      for (uint32_t i = this->tileIndex; i < MAP_LAYERS; ++i)
      {
        TileLocation &location = floor->getTileLocation(i);
        if (location.hasTile() && (location.getTile()->getItems().size() > 0 || location.getTile()->getGround()))
        {
          this->value = &location;
          this->floorIndex = z;
          this->tileIndex = i + 1;

          return this;
        }
      }
      // Reset tile index before iterating next floor
      this->tileIndex = 0;
    }
  }

  return nullptr;
}

void MapIterator::emplace(quadtree::Node *node)
{
  stack.emplace(node);
}

MapIterator Map::begin()
{
  MapIterator iterator;
  iterator.emplace(&this->root);

  while (!iterator.stack.empty())
  {
    MapIterator::NodeIndex &current = iterator.stack.top();

    if (current.node->isLeaf())
    {
      auto next = iterator.nextFromLeaf();
      return next ? *next : end();
    }

    uint32_t size = static_cast<uint32_t>(current.node->nodes.size());
    for (uint32_t i = current.cursor; i < size; ++i)
    {
      if (auto &child = current.node->nodes[i])
      {
        current.cursor = i + 1;
        iterator.emplace(child.get());
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

void MapIterator::finish()
{
  this->value = nullptr;
  this->tileIndex = UINT32_MAX;
  this->floorIndex = UINT32_MAX;
}

MapIterator &MapIterator::operator++()
{
  while (!stack.empty())
  {
    MapIterator::NodeIndex &current = stack.top();

    if (current.node->isLeaf())
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

    size_t size = current.node->nodes.size();
    // This node is finished
    if (current.cursor == size)
    {
      stack.pop();
      continue;
    }

    for (; current.cursor < size; ++current.cursor)
    {
      if (auto &child = current.node->nodes[current.cursor])
      {
        current.cursor += 1;
        emplace(child.get());
        break;
      }
    }
  }

  this->finish();
  return *this;
}

MapIterator Map::end()
{
  MapIterator iterator;
  return iterator.end();
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
  return value;
}

TileLocation *MapIterator::operator->()
{
  return value;
}

MapVersion Map::getMapVersion()
{
  return mapVersion;
}

std::string &Map::getDescription()
{
  return description;
}

void Map::createItemAt(Position pos, uint16_t id)
{
  Item item(id);

  const SpriteInfo &spriteInfo = item.itemType->appearance->getSpriteInfo();
  if (spriteInfo.hasAnimation())
  {
    auto &anim = *spriteInfo.getAnimation();
    // std::cout << anim << std::endl;
    ecs::EntityId entityId = item.assignNewEntityId();
    g_ecs.addComponent(entityId, ItemAnimationComponent(spriteInfo.getAnimation()));
  }

  getOrCreateTile(pos).addItem(std::move(item));
}
