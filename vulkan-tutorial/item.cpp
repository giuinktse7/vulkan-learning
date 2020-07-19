#include "item.h"

#include "items.h"
Item::Item(ItemTypeId itemTypeId)
		: subtype(1)
{
	this->itemType = &Items::items.getItemType(itemTypeId);
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
	uint32_t spriteId = this->itemType->appearance->frame_group().at(0).sprite_info().sprite_id(0);
	uint32_t baseOffset = spriteId - itemType->textureAtlas->firstSpriteId;

	return itemType->textureAtlas->getTextureWindow(baseOffset);
}

const bool Item::isGround() const
{
	return itemType->isGroundTile();
}