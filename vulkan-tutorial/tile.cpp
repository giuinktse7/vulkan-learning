#include "tile.h"

#include <numeric>

#include "ecs/ecs.h"
#include "ecs/item_selection.h"

Tile::Tile(TileLocation &tileLocation)
    : tileLocation(tileLocation)
{
  // cout << "Tile()" << endl;
}

Tile::Tile(Tile &&other) noexcept
    : items(std::move(other.items)),
      entity(std::move(other.entity)),
      ground(std::move(other.ground)),
      tileLocation(tileLocation){};

Tile::~Tile()
{
  if (entity.has_value())
  {
    Logger::debug() << "~Tile() with entity id: " << entity.value().id << std::endl;
    g_ecs.destroy(entity.value());
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
  if (entity.has_value())
  {
    TileSelectionComponent *component = g_ecs.getComponent<TileSelectionComponent>(entity.value());
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

const Position Tile::getPosition() const
{
  return tileLocation.position;
}

long Tile::getX() const
{
  return tileLocation.position.x;
}
long Tile::getY() const
{
  return tileLocation.position.y;
}
long Tile::getZ() const
{
  return tileLocation.position.z;
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

Entity Tile::getOrCreateEntity()
{
  if (!entity.has_value())
  {
    this->entity = g_ecs.createEntity();
  }
  return this->entity.value();
}