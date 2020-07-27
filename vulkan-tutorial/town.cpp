#include "town.h"

#include "tile.h"

Towns::Towns()
{
  ////
}

Towns::~Towns()
{
  clear();
}

void Towns::clear()
{
  towns.clear();
}

bool Towns::addTown(Town &town)
{
  std::map<uint32_t, Town>::iterator it = find(town.getID());
  if (it != end())
  {
    return false;
  }
  towns[town.getID()] = town;
  return true;
}

uint32_t Towns::getEmptyID()
{
  uint32_t empty = 0;
  for (std::map<uint32_t, Town>::iterator it = begin(); it != end(); ++it)
  {
    if (it->second.getID() > empty)
    {
      empty = it->second.getID();
    }
  }
  return empty + 1;
}

Town *Towns::getTown(std::string &name)
{
  for (std::map<uint32_t, Town>::iterator it = begin(); it != end(); ++it)
  {
    if (it->second.getName() == name)
    {
      return &it->second;
    }
  }
  return nullptr;
}

Town *Towns::getTown(uint32_t id)
{
  std::map<uint32_t, Town>::iterator it = find(id);
  if (it != end())
  {
    return &it->second;
  }

  return nullptr;
}

void Town::setTemplePosition(const Position &pos)
{
  templePosition = pos;
}
