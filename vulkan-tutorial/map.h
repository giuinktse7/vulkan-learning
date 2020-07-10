#pragma once

#include <memory>
#include <unordered_map>

#include "item.h"

class Map
{
public:
	std::unordered_map<uint32_t, std::unique_ptr<Item>> items;

	void addItem(uint32_t id)
	{
		items[id] = std::make_unique<Item>(id);
	}
};