#include "ecs.h"

#include "item_animation.h"
#include "../logger.h"

ECS g_ecs;

Entity ECS::createEntity()
{
  uint32_t id;
  if (entityIdQueue.empty())
  {
    id = entityCounter++;
  }
  else
  {
    id = entityIdQueue.front();
    entityIdQueue.pop();
  }

  entityComponentBitsets.emplace(id, ecs::ComponentBitset{});
  return {id};
}