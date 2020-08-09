#include "item_selection.h"

#include <algorithm>

#include "../type_trait.h"

#include "../graphics/engine.h"
#include "../debug.h"

constexpr TypeList<TileSelectionComponent> requiredComponents;

void TileSelectionSystem::update()
{
}

void TileSelectionSystem::deleteItems()
{
    for (const auto &entity : entities)
    {
        auto &selection = *g_ecs.getComponent<TileSelectionComponent>(entity);

        // Tile *tile = g_engine->getMapRenderer()->map->getTile(selection.position);
        TileLocation *location = g_engine->getMapRenderer()->getMap()->getTileLocation(selection.position);

        DEBUG_ASSERT(location->hasTile(), "The location has no tile.");
        Tile *tile = location->getTile();

        // All items on the tile are selected; we can delete the tile
        bool allItemsSelected = selection.itemIndices.size() == tile->getItemCount();
        if (allItemsSelected && selection.isGroundSelected())
        {
            location->removeTile();
        }
        else
        {
            for (const auto index : selection.itemIndices)
            {
                tile->removeItem(index);
            }

            if (selection.isGroundSelected())
            {
                tile->removeGround();
            }
        }
    }
}

std::vector<const char *> TileSelectionSystem::getRequiredComponents()
{
    return requiredComponents.typeNames();
}
