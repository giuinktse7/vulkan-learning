#pragma once

#include <chrono>

#include "ecs.h"
#include "../graphics/appearances.h"

namespace item_animation
{
	enum class AnimationDirection
	{
		Forward,
		Backward
	};
}

struct ItemAnimationComponent;

class ItemAnimationSystem : public ecs::System
{
public:
	void update() override;

private:
	std::vector<const char *> getRequiredComponents() override;

	void updatePingPong(ItemAnimationComponent &animation);
	void updateCounted(ItemAnimationComponent &animation);
};

struct ItemAnimationComponent
{
	ItemAnimationComponent(SpriteAnimation *animationInfo);

	void setPhase(uint32_t phaseIndex);

	SpriteAnimation *animationInfo;
	// Should not be changed after construction
	uint32_t startPhase;

	/*
		Used for async animation. Should not be changed after construction.
	*/
	std::chrono::steady_clock::time_point baseTime;

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
		std::variant<std::monostate, loop_t, item_animation::AnimationDirection> info;
		/*
			The time that the latest phase change occurred.
		*/
		std::chrono::steady_clock::time_point lastUpdateTime;
	} state;
};