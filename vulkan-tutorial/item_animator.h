#pragma once

#include "graphics/appearances.h"

#include <chrono>
#include <variant>

class ItemAnimator
{
public:
	ItemAnimator(SpriteAnimation &animation);

	SpriteAnimation &animation;

	uint32_t getPhase();

	void update();

private:
	const uint32_t startPhase;
	/*
		Used for async animation
	*/
	const std::chrono::steady_clock::time_point baseTime;

	enum class Direction
	{
		Forward,
		Backward
	};

	struct
	{
		uint32_t phaseIndex;
		/*
			Phase duration in milliseconds.
		*/
		uint32_t phaseDurationMs;

		using loop_t = uint32_t;
		/*
			Holds necessary information about the animation state based on the
			animation type.
			AnimationLoopType::Infinte -> std::monostate
			AnimationLoopType::PingPong -> ItemAnimator::Direction
			AnimationLoopType::Counted -> loop_t
		*/
		std::variant<std::monostate, loop_t, ItemAnimator::Direction> info;
		/*
			The time that the latest phase change occurred.
		*/
		std::chrono::steady_clock::time_point lastUpdateTime;
	} state;

	void setPhase(uint32_t phase);

	void updatePingPong();
	void updateCounted();
};