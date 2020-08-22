#pragma once

#include <set>
#include <functional>
#include <optional>

#include "tile.h"

// struct CompareByDrawPosition
// {
//   bool operator()(const Tile &lhs, const Tile &rhs) const
//   {
//     const Position &pos1 = lhs.getPosition();
//     const Position &pos2 = rhs.getPosition();

//     if (pos1.z != pos2.z)
//     {
//       return pos1.z > pos2.z;
//     }
//     else
//     {
//       if (pos1.x != pos2.x)
//     }
//   }
// };

struct TileHash
{
  std::size_t operator()(const Tile *tile) const noexcept
  {
    Position pos = tile->getPosition();
    size_t hash = 0;
    util::combineHash(hash, pos.x);
    util::combineHash(hash, pos.y);
    util::combineHash(hash, pos.z);

    return hash;
  }
};

struct TileEquality
{
  bool operator()(const Tile *lhs, const Tile *rhs) const
  {
    return lhs->getPosition() == rhs->getPosition();
  }
};

class Selection
{
public:
  bool blockDeselect = false;
  std::optional<Position> moveOrigin = {};
  bool moving = false;

  bool contains(Tile *tile) const;
  void select(Tile *tile);
  void addTile(Tile *tile);
  void removeTile(Tile *tile);
  void deselect(Tile *tile);

  bool empty() const;

  std::unordered_set<Tile *, TileHash, TileEquality> getTiles() const;

  void deselectAll();

  /*
  Clear the selected tiles. NOTE: This function does not deselect the tiles.
  */
  void clear();

private:
  std::unordered_set<Tile *, TileHash, TileEquality> selectedTiles;
};
