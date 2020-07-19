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

	Tile *getTile() const;

	friend class Floor;
	friend class Tile;

	void setTile(std::unique_ptr<Tile> tile);

	const Position &getPosition() const
	{
		return position;
	}

protected:
	std::unique_ptr<Tile> tile{};
	Position position;
};