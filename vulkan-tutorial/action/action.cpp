#include "action.h"

#include "../debug.h"
#include "../position.h"
#include "../map_view.h"

MapAction::MapAction(MapView &mapView, MapActionType actionType)
    : committed(false),
      mapView(mapView),
      actionType(actionType)
{
}

void MapAction::addChange(Change &&change)
{
  changes.push_back(std::move(change));
}

void MapAction::commit()
{
  DEBUG_ASSERT(!committed, "Attempted to commit an already committed action.");

  // TODO
  for (auto &change : changes)
  {
    if (change.isTileChange())
    {
      Tile &newTile = std::get<Tile>(change.data);
      Position pos = newTile.getPosition();

      // std::unique_ptr<Tile> oldTile = mapView.getMap()->replaceTile(std::move(newTile));
      // change.data = oldTile;
    }
  }
}

void MapAction::undo()
{
  if (changes.empty())
  {
    return;
  }

  // TODO
  committed = false;
}
void MapAction::redo()
{
  commit();
}

Change::Change(Tile &&tile) : data(std::move(tile))
{
}
