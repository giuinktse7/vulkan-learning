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

Item *Tile::getTopItem()
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
