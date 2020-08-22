#include "action.h"

#include <iterator>

#include "../debug.h"
#include "../position.h"
#include "../map_view.h"
#include "../util.h"

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

  Map &map = *mapView.getMap();

  for (auto &change : changes)
  {
    std::visit(util::overloaded{
                   [this, &map, &change](Tile &newTile) {
                     Position pos = newTile.getPosition();

                     std::unique_ptr<Tile> oldTilePtr = mapView.setTileInternal(std::move(newTile));

                     Tile *oldTile = oldTilePtr.release();
                     // TODO Destroy ECS components for the items of the Tile
                     change.data = std::move(*oldTile);
                   },
                   [this, &map, &change](RemovedTile &tileChange) {
                     Position &position = std::get<Position>(tileChange.data);
                     std::unique_ptr<Tile> tilePtr = mapView.removeTileInternal(position);
                     change.data = RemovedTile{std::move(*tilePtr.release())};

                     // TODO Destroy ECS components for the items of the Tile
                   },
                   [](auto &arg) {
                     ABORT_PROGRAM("Unknown change!");
                   }},
               change.data);
  }

  committed = true;
}

void MapAction::undo()
{
  if (changes.empty())
  {
    return;
  }

  for (auto it = changes.rbegin(); it != changes.rend(); ++it)
  {
    Change &change = *it;

    std::visit(util::overloaded{
                   [this, &change](Tile &oldTile) {
                     std::unique_ptr<Tile> newTilePtr = mapView.setTileInternal(std::move(oldTile));
                     Tile *newTile = newTilePtr.release();

                     // TODO Destroy ECS components for the items of the Tile

                     change.data = std::move(*newTile);
                   },

                   [this, &change](RemovedTile &tileChange) {
                     Tile &removedTile = std::get<Tile>(tileChange.data);
                     Position pos = removedTile.getPosition();

                     mapView.setTileInternal(std::move(removedTile));

                     change.data = RemovedTile{pos};
                   },

                   [](auto &arg) {
                     ABORT_PROGRAM("Unknown change!");
                   }},
               change.data);
  }

  committed = false;
}
void MapAction::redo()
{
  commit();
}

Change::Change()
    : data({})
{
}

Change::Change(Tile &&tile)
    : data(std::move(tile))
{
}

Change Change::removeTile(const Position pos)
{
  Change change;
  change.data = RemovedTile{pos};
  return change;
}

MapActionGroup::MapActionGroup(ActionGroupType groupType) : groupType(groupType)
{
}

void MapActionGroup::addAction(MapAction &&action)
{
  actions.push_back(std::move(action));
}

void MapActionGroup::commit()
{
  for (auto &action : actions)
  {
    if (!action.isCommitted())
    {
      action.commit();
    }
  }
}

void MapActionGroup::undo()
{
  for (auto it = actions.rbegin(); it != actions.rend(); ++it)
  {
    MapAction &action = *it;
    action.undo();
  }
}

MapAction *EditorHistory::getLatestAction()
{
  if (!currentGroup.has_value() || currentGroup.value().actions.empty())
  {
    return nullptr;
  }
  else
  {
    return &currentGroup.value().actions.back();
  }
}

void EditorHistory::commit(MapAction &&action)
{
  DEBUG_ASSERT(currentGroup.has_value(), "There is no current group.");

  action.commit();
  MapAction *currentAction = getLatestAction();
  if (currentAction && currentAction->getType() == action.getType())
  {
    // Same action type, can merge actions
    util::appendVector(std::move(action.changes), currentAction->changes);
  }
  else
  {
    currentGroup.value().addAction(std::move(action));
  }
}

void EditorHistory::startGroup(ActionGroupType groupType)
{
  DEBUG_ASSERT(currentGroup.has_value() == false, "The previous group was not ended.");
  Logger::debug("startGroup()");

  currentGroup.emplace(groupType);
}

void EditorHistory::endGroup(ActionGroupType groupType)
{
  DEBUG_ASSERT(currentGroup.has_value(), "There is no current group to end.");
  std::ostringstream s;
  s << "Tried to end group type " << groupType << ", but the current group type is " << currentGroup.value().groupType;
  DEBUG_ASSERT(currentGroup.value().groupType == groupType, s.str());
  Logger::debug("endGroup()");

  actionGroups.emplace(std::move(currentGroup.value()));
  currentGroup.reset();
}

void EditorHistory::undoLast()
{
  if (currentGroup.has_value())
  {
    endGroup(currentGroup.value().groupType);
  }

  if (!actionGroups.empty())
  {
    actionGroups.top().undo();
    actionGroups.pop();
  }
}

bool EditorHistory::hasCurrentGroup() const
{
  return currentGroup.has_value();
}