#include "item_animator.h"

#include "graphics/engine.h"
#include "debug.h"

uint32_t initializeStartPhase(SpriteAnimation &animation)
{
  if (animation.randomStartPhase)
  {
    return g_engine->random.nextInt(0, animation.phases.size());
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

void ItemAnimator::setPhase(uint32_t phaseIndex)
{
  SpritePhase phase = animation.phases.at(phaseIndex);

  state.phaseIndex = phaseIndex;
  state.phaseDurationMs = g_engine->random.nextInt(phase.minDuration, phase.maxDuration + 1);
  state.lastUpdateTime = g_engine->getCurrentTime();
}

uint32_t ItemAnimator::getPhase()
{
  return state.phaseIndex;
}

ItemAnimator::ItemAnimator(SpriteAnimation &animation)
    : animation(animation),
      startPhase(initializeStartPhase(animation)),
      baseTime(initializeBaseTime(animation))
{
  state.lastUpdateTime = g_engine->getCurrentTime();

  switch (animation.loopType)
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

void ItemAnimator::update()
{
  auto elapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(g_engine->getCurrentTime() - state.lastUpdateTime).count();

  if (elapsedTimeMs >= state.phaseDurationMs)
  {
    switch (animation.loopType)
    {
    case AnimationLoopType::Infinite:
      setPhase((state.phaseIndex + 1) % animation.phases.size());
      break;
    case AnimationLoopType::PingPong:
      updatePingPong();
      break;
    case AnimationLoopType::Counted:
      updateCounted();
      break;
    }
  }
}

void ItemAnimator::updatePingPong()
{
  auto direction = std::get<Direction>(state.info);
  // Last phase, reverse direction
  if (state.phaseIndex == 0 || state.phaseIndex == animation.phases.size() - 1)
  {
    direction = direction == Direction::Forward ? Direction::Backward : Direction::Forward;
  }

  uint32_t indexChange = direction == Direction::Forward ? 1 : -1;
  setPhase(state.phaseIndex + indexChange);
}

void ItemAnimator::updateCounted()
{
  uint32_t currentLoop = std::get<uint32_t>(state.info);
  if (currentLoop != animation.loopCount)
  {
    if (state.phaseIndex == animation.phases.size() - 1)
    {
      state.info = currentLoop + 1;
      DEBUG_ASSERT(std::get<uint32_t>(state.info) <= animation.loopCount, "The current loop for an animation cannot be higher than its loopCount.");
    }

    setPhase(0);
  }
}