#include "item_selection.h"

#include <algorithm>

#include "../type_trait.h"

#include "../graphics/engine.h"
#include "../debug.h"

constexpr TypeList<TileSelectionComponent> requiredComponents;

void TileSelectionSystem::update()
{
}

void TileSelectionSystem::clearAllSelections()
{
  g_ecs.removeAllComponents<TileSelectionComponent>();
}

void TileSelectionSystem::deleteItems()
{
  MapView *mapView = g_engine->getMapView();

  mapView->history.startGroup(ActionGroupType::RemoveMapItem);
  for (const auto &entity : entities)
  {
    auto &selection = *g_ecs.getComponent<TileSelectionComponent>(entity);
    const Position position = selection.position;

    if (selection.isAllSelected())
    {
      mapView->removeTile(position);
    }
    else
    {
      if (!selection.itemIndices.empty())
      {
        mapView->removeItems(position, selection.itemIndices);
      }

      if (selection.isGroundSelected())
      {
        mapView->removeGround(position);
      }
    }
  }

  g_ecs.destroyMarkedEntities();
  g_ecs.removeAllComponents<TileSelectionComponent>();
  mapView->history.endGroup(ActionGroupType::RemoveMapItem);
}

std::vector<const char *> TileSelectionSystem::getRequiredComponents()
{
  return requiredComponents.typeNames();
}
