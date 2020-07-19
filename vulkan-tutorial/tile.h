#pragma once

#include "tile_location.h"
#include "item.h"

#include <memory>

class TileLocation;

class Tile
{
public:
	Tile(TileLocation &location);
	~Tile();

	Tile(const Tile &) = delete;
	Tile &operator=(const Tile &) = delete;

	Item *getTopItem() const;
	Item *getGround() const;

	void addItem(std::unique_ptr<Item> item);

	const std::vector<std::unique_ptr<Item>> &getItems() const
	{
		return items;
	}

	const Position &getPosition() const;

private:
	TileLocation &tileLocation;
	std::unique_ptr<Item> ground;
	std::vector<std::unique_ptr<Item>> items;
};