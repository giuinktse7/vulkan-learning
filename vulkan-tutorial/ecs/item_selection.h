#pragma once

#include <bitset>

#include "ecs.h"
#include "../position.h"
#include "../item.h"
#include "../util.h"

// Entities excluding items. E.g. ground, creature, spawn.
constexpr size_t MaxDistinctTileEntities = 16;

enum class TileEntity
{
  Ground = 0,
  Creature = 1
};

class TileSelectionSystem : public ecs::System
{
public:
  void update() override;

  void deleteItems();

private:
  std::vector<const char *> getRequiredComponents() override;
};

struct TileSelectionComponent
{
  Position position;
  /* Important: This has to be in descending order. Otherwise item deletion will 
    offset consecutive indices, resulting in removal of incorrect items.
  */
  std::set<size_t, std::greater<size_t>> itemIndices;

  void select(TileEntity tileEntity)
  {
    selectedEntities.set(to_underlying(tileEntity));
  }

  void deselect(TileEntity tileEntity)
  {
    selectedEntities.reset(to_underlying(tileEntity));
  }

  void toggleSelection(TileEntity tileEntity)
  {
    selectedEntities.flip(to_underlying(tileEntity));
  }

  void selectItemIndex(size_t index)
  {
    itemIndices.emplace(index);
  }
  void deselectItemIndex(size_t index)
  {
    itemIndices.emplace(index);
  }

  void toggleItemSelection(size_t index)
  {
    bool selected = itemIndices.find(index) != itemIndices.end();
    if (selected)
    {
      deselectItemIndex(index);
    }
    else
    {
      selectItemIndex(index);
    }
  }

  bool isGroundSelected() const
  {
    return selectedEntities.test(to_underlying(TileEntity::Ground));
  }

private:
  std::bitset<16> selectedEntities;
};
