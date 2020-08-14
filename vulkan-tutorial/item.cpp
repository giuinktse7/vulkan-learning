#include "item.h"

#include "items.h"
#include "ecs/ecs.h"
#include "ecs/item_animation.h"

#include "graphics/engine.h"
Item::Item(ItemTypeId itemTypeId)
		: subtype(1)
{
	this->itemType = Items::items.getItemType(itemTypeId);
}

// std::optional<Entity> entity;
// ItemType *itemType;
// std::unordered_map<ItemAttribute_t, ItemAttribute> attributes;
// uint16_t subtype = 1;

Item::Item(Item &&other) noexcept
		: entity(std::move(other.entity)),
			itemType(other.itemType),
			attributes(std::move(other.attributes)),
			subtype(other.subtype)
{
	other.entity = {};
}

Item &Item::operator=(Item &&other) noexcept
{
	entity = std::move(other.entity);
	itemType = other.itemType;
	attributes = std::move(other.attributes);
	subtype = other.subtype;

	other.entity = {};

	return *this;
}

Item::~Item()
{
	if (entity.has_value())
	{
		g_ecs.destroy(entity.value());
		std::cout << "Destroying item " << std::to_string(this->getId()) << "(" << this->getName() << "), entity id: " << entity.value().id << std::endl;
	}
	else
	{
		std::cout << "Destroying item " << std::to_string(this->getId()) << "(" << this->getName() << "), entity: None" << std::endl;
	}
}

const TextureInfo Item::getTextureInfo(const Position &pos) const
{
	// TODO Add more pattern checks like hanging or cumulative item types
	const SpriteInfo &spriteInfo = itemType->appearance->getSpriteInfo();
	if (spriteInfo.hasAnimation() && this->entity.has_value())
	{
		auto c = g_ecs.getComponent<ItemAnimationComponent>(this->entity.value());

		uint32_t width = spriteInfo.patternWidth;
		uint32_t height = spriteInfo.patternHeight;
		uint32_t depth = spriteInfo.patternDepth;

		uint32_t patternIndex = itemType->getPatternIndex(pos);
		uint32_t spriteIndex = patternIndex + c->state.phaseIndex * width * height * depth;

		uint32_t spriteId = spriteInfo.spriteIds.at(spriteIndex);

		return itemType->getTextureInfo(spriteId);
	}
	return itemType->getTextureInfo(pos);
}

const bool Item::isGround() const
{
	return itemType->isGroundTile();
}

uint16_t Item::getSubtype() const
{
	if (itemType->hasSubType())
	{
		return subtype;
	}

	return 0;
}
