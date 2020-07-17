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

	const TextureWindow getTextureWindow() const;

	ItemType *itemType;

private:
	// Subtype is either fluid type, count, subtype, or charges.
	uint16_t subtype;
};