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

	/*
		Counts all entities (items, creature, spawn, waypoint, etc.).
	*/
	size_t getEntityCount();

	const Position &getPosition() const;
	const uint32_t &getX() const;
	const uint32_t &getY() const;
	const uint32_t &getZ() const;

	inline uint16_t Tile::getMapFlags() const;
	inline uint16_t Tile::getStatFlags() const;

private:
	TileLocation &tileLocation;
	std::unique_ptr<Item> ground;
	std::vector<std::unique_ptr<Item>> items;

	// This structure makes it possible to access all flags, or map/stat flags separately.
	union
	{
		struct
		{
			uint16_t mapflags;
			uint16_t statflags;
		};
		uint32_t flags;
	};
};