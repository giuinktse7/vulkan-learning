#pragma once

#include "graphics/appearances.h"

class ItemAnimator
{
public:
	ItemAnimator(SpriteAnimation &animation);

	uint32_t getStartPhase() const;

	SpriteAnimation &animation;
};