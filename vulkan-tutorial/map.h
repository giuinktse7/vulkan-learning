#pragma once

#include <memory>
#include <unordered_map>

#include "item.h"
#include "tile.h"
#include "quad_tree.h"

class Map
{
public:
	Map();
	std::unordered_map<uint32_t, std::unique_ptr<Item>> items;

	Tile &createTile(int x, int y, int z);

	void addItem(uint32_t id)
	{
		items[id] = std::make_unique<Item>(id);
	}

private:
	quadtree::Node root;
};