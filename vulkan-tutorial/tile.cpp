#include "tile.h"

#include <numeric>

#include "ecs/ecs.h"
#include "ecs/item_selection.h"
#include "tile_location.h"

Tile::Tile(TileLocation &tileLocation)
    : position(tileLocation.position)
{
}

Tile::Tile(Position position)
    : position(position) {}

// TODO BUG? It is possible that entityId from OptionalEntity does not get moved correctly.
// TODO It should be deleted from "other" and put into the newly constructed Tile.
Tile::Tile(Tile &&other) noexcept
    : ecs::OptionalEntity(std::move(other)),
      items(std::move(other.items)),
      ground(std::move(other.ground)),
      position(other.position)
{
  other.entityId.reset();
}

Tile &Tile::operator=(Tile &&other) noexcept
{
  items = std::move(other.items);
  entityId = std::move(other.entityId);
  ground = std::move(other.ground);
  position = std::move(other.position);

  other.entityId = {};

  return *this;
}

Tile::~Tile()
{
  if (isEntity())
  {
    Logger::debug() << "~Tile() with entity id: " << getEntityId().value() << std::endl;
    g_ecs.destroy(getEntityId().value());
  }
}

Item *Tile::getGround() const
{
  return ground.get();
}

const Item *Tile::getTopItem() const
{
  if (items.size() > 0)
  {
    return &items.back();
  }
  if (ground)
  {
    return ground.get();
  }

  return nullptr;
}

void Tile::addItem(Item &&item)
{
  std::cout << "Toporder: " << item.getTopOrder() << std::endl;

  if (item.isGround())
  {
    this->ground = std::make_unique<Item>(std::move(item));
    return;
  }

  std::vector<Item>::iterator cursor;

  if (item.itemType->alwaysOnTop)
  {
    cursor = items.begin();
    while (cursor != items.end())
    {
      if (cursor->itemType->alwaysOnTop)
      {
        if (item.getTopOrder() < cursor->getTopOrder())
        {
          break;
        }
      }
      else
      {
        break;
      }
      ++cursor;
    }
  }
  else
  {
    cursor = items.end();
  }

  items.insert(cursor, std::move(item));
  updateSelectionComponent();
}

void Tile::updateSelectionComponent() const
{
  if (isEntity())
  {
    TileSelectionComponent *component = g_ecs.getComponent<TileSelectionComponent>(getEntityId().value());
    if (component)
    {
      component->tileItemCount = items.size();
    }
  }
}

void Tile::removeItem(size_t index)
{
  auto pos = items.begin();
  std::advance(pos, index);
  items.erase(pos);

  updateSelectionComponent();
}

void Tile::removeGround()
{
  this->ground.reset();
}

size_t Tile::getEntityCount()
{
  size_t result = items.size();
  if (ground)
    ++result;

  return result;
}

int Tile::getTopElevation() const
{
  return std::accumulate(
      items.begin(),
      items.end(),
      0,
      [](int elevation, const Item &next) { return elevation + next.itemType->getElevation(); });
}

ecs::EntityId Tile::getOrCreateEntity()
{
  if (!isEntity())
  {
    assignNewEntityId();
  }

  return this->getEntityId().value();
}

Tile Tile::deepCopy() const
{
  Tile tile(this->position);
  for (const auto &item : this->items)
  {
    tile.addItem(item.deepCopy());
  }
  tile.flags = this->flags;
  if (this->getGround()) {
    tile.ground = std::make_unique<Item>(this->getGround()->deepCopy());
  }

  return tile;
}

void Tile::setLocation(TileLocation &location)
{
  this->position = location.position;
}

const Position Tile::getPosition() const
{
  return position;
}

long Tile::getX() const
{
  return position.x;
}
long Tile::getY() const
{
  return position.y;
}
long Tile::getZ() const
{
  return position.z;
}