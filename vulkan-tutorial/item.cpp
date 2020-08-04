#include "item.h"

#include "items.h"

#include "graphics/engine.h"
Item::Item(ItemTypeId itemTypeId)
		: subtype(1)
{
	this->itemType = Items::items.getItemType(itemTypeId);
}
Item::~Item()
{
	// std::cout << "Destroying item " << std::to_string(this->getId()) << "(" << this->getName() << std::endl;
}

std::unique_ptr<Item> Item::create(ItemTypeId serverId)
{
	return std::make_unique<Item>(serverId);
}

const TextureInfo Item::getTextureInfo(const Position &pos) const
{
	// TODO Add more pattern checks like hanging or cumulative item types

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
