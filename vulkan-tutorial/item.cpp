#include "item.h"

#include "items.h"
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

const TextureWindow Item::getTextureWindow() const
{
	// TODO get correct texture window based on item offset, subtype, etc.
	return itemType->getTextureWindow();
}

const glm::vec2 Item::getTextureAtlasSize() const
{
	return {itemType->getTextureAtlas()->width, itemType->getTextureAtlas()->height};
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
