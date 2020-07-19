#include "tile.h"

using namespace std;

Tile::Tile(TileLocation &tileLocation)
    : tileLocation(tileLocation)
{
  cout << "Tile()" << endl;
}

Tile::~Tile()
{
  cout << "~Tile()" << endl;
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

const Position &Tile::getPosition() const
{
  return tileLocation.position;
}