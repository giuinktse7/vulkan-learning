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

	std::unique_ptr<Tile> replaceTile(Tile &&tile);

	Tile *getTile() const;
	const bool hasTile() const;

	void removeTile();

	friend class Floor;
	friend class Tile;

	void setTile(std::unique_ptr<Tile> tile);

	const Position getPosition() const;
	long getX() const;
	long getY() const;
	long getZ() const;

protected:
	std::unique_ptr<Tile> tile{};
	Position position;
};