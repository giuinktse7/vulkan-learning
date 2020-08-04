#include "item_animator.h"

ItemAnimator::ItemAnimator(SpriteAnimation &animation)
    : animation(animation)
{
}

uint32_t ItemAnimator::getStartPhase() const
{
  if (animation.randomStartPhase)
  {
    }
  return animation.defaultStartPhase;
}
