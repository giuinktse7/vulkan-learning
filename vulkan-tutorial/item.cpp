#include "item.h"

#include "items.h"

Item::Item(uint32_t itemTypeId)
{
	this->itemType = &Items::items.getItemType(itemTypeId);
}

const TextureWindow Item::getTextureWindow() const
{
	uint32_t spriteId = this->itemType->appearance->frame_group().at(0).sprite_info().sprite_id(0);
	uint32_t baseOffset = spriteId - itemType->textureAtlas->firstSpriteId;

	return itemType->textureAtlas->getTextureWindow(baseOffset);
}