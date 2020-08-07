#include "item_animation.h"

#include "../type_trait.h"
#include "../graphics/engine.h"
#include "../debug.h"

constexpr TypeList<ItemAnimationComponent> requiredComponents;

using Direction = item_animation::AnimationDirection;

uint32_t initializeStartPhase(SpriteAnimation &animation)
{
  if (animation.randomStartPhase)
  {
    return g_engine->random.nextInt<uint32_t>(0, static_cast<int>(animation.phases.size()));
  }
  return animation.defaultStartPhase;
}

std::chrono::steady_clock::time_point initializeBaseTime(SpriteAnimation &animation)
{
  if (animation.synchronized)
  {
    return g_engine->getStartTime();
  }
  else
  {
    return TimeMeasure::getCurrentTime();
  }
}

ItemAnimationComponent::ItemAnimationComponent(SpriteAnimation *animationInfo)
    : animationInfo(animationInfo),
      startPhase(initializeStartPhase(*animationInfo)),
      baseTime(initializeBaseTime(*animationInfo))
{
  state.lastUpdateTime = g_engine->getCurrentTime();

  switch (animationInfo->loopType)
  {
  case AnimationLoopType::Infinite:
    state.info = {};
    break;
  case AnimationLoopType::PingPong:
    state.info = Direction::Forward;
    break;
  case AnimationLoopType::Counted:
    state.info = 0;
  }

  setPhase(0);
}

void ItemAnimationComponent::setPhase(uint32_t phaseIndex)
{
  SpritePhase phase = animationInfo->phases.at(phaseIndex);

  state.phaseIndex = phaseIndex;
  state.phaseDurationMs = g_engine->random.nextInt<uint32_t>(phase.minDuration, phase.maxDuration + 1);
  state.lastUpdateTime = g_engine->getCurrentTime();
}

std::vector<const char *> ItemAnimationSystem::getRequiredComponents()
{
  return requiredComponents.typeNames();
}

void ItemAnimationSystem::update()
{
  for (const auto &entity : entities)
  {
    auto &animation = g_ecs.getComponent<ItemAnimationComponent>(entity);

    auto elapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(g_engine->getCurrentTime() - animation.state.lastUpdateTime).count();

    if (elapsedTimeMs >= animation.state.phaseDurationMs)
    {
      switch (animation.animationInfo->loopType)
      {
      case AnimationLoopType::Infinite:
        animation.setPhase((static_cast<size_t>(animation.state.phaseIndex) + 1) % animation.animationInfo->phases.size());
        break;
      case AnimationLoopType::PingPong:
        updatePingPong(animation);
        break;
      case AnimationLoopType::Counted:
        updateCounted(animation);
        break;
      }
    }
  }
}

void ItemAnimationSystem::updatePingPong(ItemAnimationComponent &animation)
{
  auto direction = std::get<Direction>(animation.state.info);
  // Last phase, reverse direction
  if (animation.state.phaseIndex == 0 || animation.state.phaseIndex == animation.animationInfo->phases.size() - 1)
  {
    direction = direction == Direction::Forward ? Direction::Backward : Direction::Forward;
  }

  uint32_t indexChange = direction == Direction::Forward ? 1 : -1;
  animation.setPhase(animation.state.phaseIndex + indexChange);
}

void ItemAnimationSystem::updateCounted(ItemAnimationComponent &animation)
{
  uint32_t currentLoop = std::get<uint32_t>(animation.state.info);
  if (currentLoop != animation.animationInfo->loopCount)
  {
    if (animation.state.phaseIndex == animation.animationInfo->phases.size() - 1)
    {
      animation.state.info = currentLoop + 1;
      DEBUG_ASSERT(std::get<uint32_t>(animation.state.info) <= animation.animationInfo->loopCount, "The current loop for an animation cannot be higher than its loopCount.");
    }

    animation.setPhase(0);
  }
}