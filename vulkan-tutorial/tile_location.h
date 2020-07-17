#pragma once

#include "tile.h"
#include "position.h"

class Tile;

class TileLocation
{
	TileLocation();

public:
	~TileLocation();

	TileLocation(const TileLocation &) = delete;
	TileLocation &operator=(const TileLocation &) = delete;

	Tile *get()
	{
		return tile ? tile.get() : nullptr;
	}

	friend class Floor;

	void setTile(std::unique_ptr<Tile> tile);

protected:
	std::unique_ptr<Tile> tile{};
	Position position;
};