#pragma once

#include <vector>
#include <deque>

#include "../tile.h"

class Change;
class MapView;

enum class MapActionType
{
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

  void addChange(Change &&change);

  void commit();

  void undo();
  void redo();

  bool isCommitted() const
  {
    return committed;
  }

private:
  bool committed;

  MapActionType actionType;
  MapView &mapView;
  std::vector<Change> changes;
};

class MapActionGroup
{
private:
  std::vector<MapAction> actions;
};

class Change
{
public:
  Change(Tile &&tile);
  /*
    If the change has not yet been committed, this member contains the new data.
    If the change has been committed, this member contains the old data, i.e.
    the data necessary to undo the change.
  */
  std::variant<Tile> data;

  bool isTileChange() const
  {
    // return std::holds_alternative<Tile>(data);
    return false;
  }
};

class EditorHistory
{

private:
  std::deque<MapActionGroup> actionGroups;
};