#pragma once

#include <vector>
#include <stack>
#include <unordered_map>

#include "../tile_location.h"
#include "../ecs/item_animation.h"
#include "../ecs/item_selection.h"

class Change;
class MapView;
class EditorHistory;

enum class ActionGroupType
{
  AddMapItem
};

enum class ChangeType
{
  Tile
};

enum class MapActionType
{
  CreateTiles,
  Move,
  Select,
  DeleteTiles,
  CutTiles,
  PasteTiles
};

class MapAction
{
public:
  MapAction(MapView &mapView, MapActionType actionType);

  MapAction(const MapAction &other) = delete;
  MapAction &operator=(const MapAction &other) = delete;

  MapAction(MapAction &&other) noexcept
      : committed(other.committed),
        actionType(other.actionType),
        mapView(other.mapView),
        changes(std::move(other.changes))
  {
  }

  void addChange(Change &&change);

  void commit();

  void undo();
  void redo();

  bool isCommitted() const
  {
    return committed;
  }

  MapActionType getType() const
  {
    return actionType;
  }

private:
  friend class EditorHistory;
  bool committed;

  MapActionType actionType;
  MapView &mapView;
  std::vector<Change> changes;
};

/*
  Represents a group of actions. One undo command will undo all the actions in
  the group.
*/
class MapActionGroup
{
public:
  MapActionGroup(ActionGroupType groupType);
  void addAction(MapAction &&action);

  MapActionGroup(MapActionGroup &&other) noexcept
      : groupType(other.groupType),
        actions(std::move(other.actions))
  {
  }

  void commit();
  void undo();

  ActionGroupType groupType;

private:
  friend class EditorHistory;
  std::vector<MapAction> actions;
};

struct ECSTileComponent
{
  std::variant<uint32_t, bool> data;
  // std::variant<TileSelectionComponent, ItemAnimationComponent> data;
};

class Change
{
public:
  Change(Tile &&tile);

  Change(const Change &other) = delete;
  Change &operator=(const Change &other) = delete;

  Change(Change &&other) noexcept
      : changeType(other.changeType),
        data(std::move(other.data))
  {
  }

  Change &operator=(Change &&other)
  {
    changeType = other.changeType;
    data = std::move(other.data);
    return *this;
  }

  ChangeType changeType;

  /*
    If the change has not yet been committed, this member contains the new data.
    If the change has been committed, this member contains the old data, i.e.
    the data necessary to undo the change.
  */
  std::variant<Tile, ECSTileComponent> data;

  bool isTileChange() const
  {
    return std::holds_alternative<Tile>(data);
  }

  bool isEcsComponentChange() const
  {
    return std::holds_alternative<ECSTileComponent>(data);
  }
};

class EditorHistory
{
public:
  void commit(MapAction &&action);
  void undoLast();

  void startGroup(ActionGroupType groupType);
  void endGroup(ActionGroupType groupType);

  bool hasCurrentGroup() const;

private:
  std::optional<MapActionGroup> currentGroup;
  std::stack<MapActionGroup> actionGroups;

  MapAction *getLatestAction();
};

inline std::ostream &operator<<(std::ostream &os, ActionGroupType type)
{
  switch (type)
  {
  case ActionGroupType::AddMapItem:
    os << "KeyState::Release";
    break;
  default:
    os << "Unknown ActionGroupType: " << to_underlying(type);
    break;
  }

  return os;
}
