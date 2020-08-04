#pragma once

#include <stdint.h>
#include "graphics/texture.h"
#include "graphics/appearances.h"
#include <glm/glm.hpp>

#include "item_attribute.h"

#include "items.h"
#include "graphics/texture_atlas.h"
#include "position.h"

#include <unordered_map>

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

	const TextureInfo getTextureInfo(const Position &pos) const;

	const bool isGround() const;

	uint16_t getSubtype() const;

	bool hasAttributes() const
	{
		return attributes.size() > 0;
	}

	const inline int getTopOrder() const
	{
		return itemType->alwaysOnTopOrder;
	}

	const std::unordered_map<ItemAttribute_t, ItemAttribute> &getAttributes()
	{
		return attributes;
	}

	ItemType *itemType;

private:
	std::unordered_map<ItemAttribute_t, ItemAttribute> attributes;
	// Subtype is either fluid type, count, subtype, or charges.
	uint16_t subtype = 1;

	Item &operator=(const Item &i) = delete;
	Item(const Item &i) = delete;
	Item &operator==(const Item &i) = delete;
};