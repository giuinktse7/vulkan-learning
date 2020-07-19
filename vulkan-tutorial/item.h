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
	~Item();

	Item(Item &&item) = default;
	Item &operator=(Item &&item) = default;

	static std::unique_ptr<Item> create(ItemTypeId serverId);

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

	const DrawOffset getDrawOffset()
	{
		return itemType->textureAtlas->drawOffset;
	}

	const uint32_t getWidth() const
	{
		return itemType->textureAtlas->spriteWidth;
	}

	const uint32_t getHeight() const
	{
		return itemType->textureAtlas->spriteHeight;
	}

	const TextureWindow getTextureWindow() const;

	const bool isGround() const;

	const inline int getTopOrder() const
	{
		return itemType->alwaysOnTopOrder;
	}

	ItemType *itemType;

private:
	// Subtype is either fluid type, count, subtype, or charges.
	uint16_t subtype = 1;

	Item &operator=(const Item &i) = delete;
	Item(const Item &i) = delete;
	Item &operator==(const Item &i) = delete;
};