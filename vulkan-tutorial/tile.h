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

	Item *getTopItem();

private:
	TileLocation &tileLocation;
	std::unique_ptr<Item> ground;
	std::vector<Item> items;
};