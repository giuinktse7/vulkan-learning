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
  void clearAllSelections();

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

  size_t tileItemCount;

  bool isAllSelected() const
  {
    if (allSelected)
    {
      return true;
    }
    else
    {
      bool allItemsSelected = itemIndices.size() == tileItemCount;
      return allItemsSelected && isGroundSelected();
    }
  }

  bool isGroundSelected() const
  {
    return allSelected || selectedEntities.test(to_underlying(TileEntity::Ground));
  }

  bool isItemIndexSelected(size_t index) const
  {
    return allSelected || itemIndices.find(index) != itemIndices.end();
  }

  void selectAll()
  {
    allSelected = true;
  }

  void toggleSelectAll()
  {
    if (!allSelected)
    {
      allSelected = true;
    }
    else
    {
      allSelected = false;
      selectedEntities.reset();
      itemIndices.clear();
    }
  }

  void select(TileEntity tileEntity)
  {
    selectedEntities.set(to_underlying(tileEntity));
  }

  void deselect(TileEntity tileEntity)
  {
    if (allSelected)
    {
      allSelected = false;
    }
    selectedEntities.reset(to_underlying(tileEntity));
  }

  void toggleSelection(TileEntity tileEntity)
  {
    selectedEntities.flip(to_underlying(tileEntity));
    if (!selectedEntities.test(to_underlying(tileEntity)))
    {
      if (allSelected)
      {
        allSelected = false;
        selectAllItems();
        selectAllTileEntities();
        deselect(tileEntity);
      }
    }
  }

  void selectItemIndex(size_t index)
  {
    itemIndices.emplace(index);
  }

  void deselectItemIndex(size_t index)
  {
    if (allSelected)
    {
      allSelected = false;

      selectAllItems();
      selectAllTileEntities();
    }

    itemIndices.erase(index);
  }

  void toggleItemSelection(size_t index)
  {
    if (isItemIndexSelected(index))
    {
      deselectItemIndex(index);
    }
    else
    {
      selectItemIndex(index);
    }
  }

  bool isTopSelected() const
  {
    if (allSelected)
    {
      return true;
    }
    else if (tileItemCount > 0)
    {
      return isItemIndexSelected(tileItemCount - 1);
    }
    else
    {
      return isGroundSelected();
    }
  }

private:
  std::bitset<16> selectedEntities;

  void selectAllItems()
  {
    for (size_t i = 0; i < tileItemCount; ++i)
    {
      itemIndices.emplace(i);
    }
  }

  void selectAllTileEntities()
  {
    select(TileEntity::Ground);
  }

  /*
    IMPORTANT: Has to be set to false whenever something is deselected.
  */
  bool allSelected = false;
};
