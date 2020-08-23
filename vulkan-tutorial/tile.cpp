#include "tile.h"

#include <numeric>

#include "ecs/ecs.h"
#include "tile_location.h"

Tile::Tile(TileLocation &tileLocation)
    : position(tileLocation.position), selectionCount(0)
{
}

Tile::Tile(Position position)
    : position(position), selectionCount(0) {}

// TODO BUG? It is possible that entityId from OptionalEntity does not get moved correctly.
// TODO It should be deleted from "other" and put into the newly constructed Tile.
Tile::Tile(Tile &&other) noexcept
    : items(std::move(other.items)),
      ground(std::move(other.ground)),
      position(other.position),
      selectionCount(other.selectionCount)
{
}

Tile &Tile::operator=(Tile &&other) noexcept
{
  items = std::move(other.items);
  ground = std::move(other.ground);
  position = std::move(other.position);
  selectionCount = other.selectionCount;

  return *this;
}

Tile::~Tile()
{
}

Item *Tile::getGround() const
{
  return ground.get();
}

bool Tile::hasTopItem() const
{
  return !isEmpty();
}

Item *Tile::getTopItem() const
{
  if (items.size() > 0)
  {
    return const_cast<Item *>(&items.back());
  }
  if (ground)
  {
    return ground.get();
  }

  return nullptr;
}

bool Tile::topItemSelected() const
{
  if (!hasTopItem())
    return false;

  const Item *topItem = getTopItem();
  return allSelected() || topItem->selected;
}

void Tile::removeItem(size_t index)
{
  if (index == items.size() - 1)
  {
    items.pop_back();
  }
  else
  {
    items.erase(items.begin() + index);
  }
}

void Tile::deselectAll()
{
  if (ground)
    ground->selected = false;

  for (Item &item : items)
  {
    item.selected = false;
  }

  selectionCount = 0;
}

void Tile::addItem(Item &&item)
{
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
        if (item.itemType->isGroundBorder())
        {
          if (!cursor->itemType->isGroundBorder())
          {
            break;
          }
        }
        else // New item is not a border
        {
          if (cursor->itemType->alwaysOnTop)
          {
            if (!cursor->itemType->isGroundBorder())
            {
              // Replace the current item at cursor with the new item
              *(cursor) = std::move(item);
              return;
            }
          }
        }
        // if (item.getTopOrder() < cursor->getTopOrder())
        // {
        //   break;
        // }
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

Tile Tile::deepCopy() const
{
  Tile tile(this->position);
  for (const auto &item : this->items)
  {
    tile.addItem(item.deepCopy());
  }
  tile.flags = this->flags;
  if (this->getGround())
  {
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

bool Tile::isEmpty() const
{
  return !ground && items.empty();
}

bool Tile::allSelected() const
{
  size_t size = items.size();
  if (ground)
    ++size;

  return selectionCount == size;
}

bool Tile::hasSelection() const
{
  return selectionCount != 0;
}

void Tile::selectItemAtIndex(size_t index)
{
  items.at(index).selected = true;
}

void Tile::deselectItemAtIndex(size_t index)
{
  items.at(index).selected = false;
}

void Tile::selectAll()
{
  size_t count = 0;
  if (ground)
  {
    ++count;
    ground->selected = true;
  }

  count += items.size();
  for (Item &item : items)
  {
    item.selected = true;
  }

  selectionCount = count;
}

void Tile::selectTopItem()
{
  if (items.empty())
  {
    if (ground)
    {
      if (!ground->selected)
      {
        ++selectionCount;
      }
      ground->selected = true;
    }
  }
  else
  {
    if (!items.back().selected)
    {
      ++selectionCount;
    }

    items.back().selected = true;
  }
}

void Tile::deselectTopItem()
{
  if (items.empty())
  {
    if (ground)
    {
      if (ground->selected)
      {
        --selectionCount;
      }
      ground->selected = false;
    }
  }
  else
  {
    if (items.back().selected)
    {
      --selectionCount;
    }

    items.back().selected = false;
  }
}
