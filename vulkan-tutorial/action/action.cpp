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

  for (auto &change : changes)
  {
    switch (change.changeType)
    {
    case ChangeType::Tile:
    {
      Tile &newTile = std::get<Tile>(change.data);

      std::unique_ptr<Tile> oldTilePtr = mapView.getMap()->replaceTile(std::move(newTile));
      Tile *oldTile = oldTilePtr.release();
      // TODO Also store ECS component changes
      oldTile->destroyEntity();
      change.data = std::move(*oldTile);

      break;
    }
    default:
      ABORT_PROGRAM("Unknown ChangeType.");
    }
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

    switch (change.changeType)
    {
    case ChangeType::Tile:
    {
      Tile &oldTile = std::get<Tile>(change.data);

      std::unique_ptr<Tile> newTilePtr = mapView.getMap()->replaceTile(std::move(oldTile));
      Tile *newTile = newTilePtr.release();
      // TODO Also store ECS component changes
      newTile->destroyEntity();
      change.data = std::move(*newTile);

      break;
    }
    default:
      ABORT_PROGRAM("Unknown ChangeType.");
    }
  }

  committed = false;
}
void MapAction::redo()
{
  commit();
}

Change::Change(Tile &&tile)
    : changeType(ChangeType::Tile),
      data(std::move(tile))
{
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