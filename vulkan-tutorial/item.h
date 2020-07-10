#pragma once

#include <stdint.h>
#include "graphics/texture.h"
#include "graphics/appearances.h"

#include "items.h"
#include "graphics/texture_atlas.h"

class Appearances;

class Item
{
	using ItemTypeId = uint32_t;

public:
	Item(ItemTypeId serverId);

	uint32_t getId() const
	{
		return itemType->id;
	}

	uint32_t getClientId() const
	{
		return itemType->clientId;
	}

	const std::string getName() const
	{
		return itemType->name;
	}

	const uint32_t getWeight() const
	{
		return itemType->weight;
	}

	const TextureWindow getTextureWindow() const
	{
		auto appearance = Appearances::getById(getClientId());
		uint32_t spriteId = appearance.frame_group().at(0).sprite_info().sprite_id(0);
		uint32_t baseOffset = spriteId - itemType->textureAtlas->firstSpriteId;

		return itemType->textureAtlas->getTextureWindow(baseOffset);
	}

	ItemType *itemType;

private:
};