#include "tile.h"

using namespace std;

Tile::Tile(TileLocation &tileLocation)
    : tileLocation(tileLocation)
{
  // cout << "Tile()" << endl;
}

Tile::~Tile()
{
  // cout << "~Tile()" << endl;
}

Item *Tile::getGround() const
{
  return ground.get();
}

Item *Tile::getTopItem() const
{
  if (items.size() > 0)
  {
    return items.back().get();
  }
  if (ground)
  {
    return ground.get();
  }

  return nullptr;
}

void Tile::addItem(std::unique_ptr<Item> item)
{
  if (item->isGround())
  {
    this->ground = std::move(item);
    return;
  }

  std::vector<std::unique_ptr<Item>>::iterator cursor;

  if (item->itemType->alwaysOnTop)
  {
    cursor = items.begin();
    while (cursor != items.end())
    {
      if ((*cursor)->itemType->alwaysOnTop)
      {
        if (item->getTopOrder() < (*cursor)->getTopOrder())
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

  // TODO Is the move unnecessary?
  items.insert(cursor, std::move(item));
}

void Tile::removeItem(size_t index)
{
  auto pos = items.begin();
  std::advance(pos, index);
  items.erase(pos);
}

void Tile::removeGround()
{
  this->ground.reset();
}

const Position &Tile::getPosition() const
{
  return tileLocation.position;
}

const uint32_t &Tile::getX() const
{
  return tileLocation.position.x;
}
const uint32_t &Tile::getY() const
{
  return tileLocation.position.y;
}
const uint32_t &Tile::getZ() const
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