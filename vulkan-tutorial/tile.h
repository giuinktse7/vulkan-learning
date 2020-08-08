#pragma once

#include <memory>
#include <optional>

#include "tile_location.h"
#include "item.h"
#include "ecs/entity.h"

class TileLocation;

class Tile
{
public:
	std::optional<Entity> entity;

	Tile(TileLocation &location);
	~Tile();

	Tile(const Tile &) = delete;
	Tile &operator=(const Tile &) = delete;

	Item *getTopItem() const;
	Item *getGround() const;

	void addItem(std::unique_ptr<Item> item);
	void removeItem(size_t index);
	void removeGround();

	const std::vector<std::unique_ptr<Item>> &getItems() const
	{
		return items;
	}

	const size_t getItemCount() const
	{
		return items.size();
	}

	/*
		Counts all entities (items, creature, spawn, waypoint, etc.).
	*/
	size_t getEntityCount();

	const Position &getPosition() const;
	const uint32_t &getX() const;
	const uint32_t &getY() const;
	const uint32_t &getZ() const;

	uint16_t getMapFlags() const;
	uint16_t getStatFlags() const;

private:
	TileLocation &tileLocation;
	std::unique_ptr<Item> ground;
	std::vector<std::unique_ptr<Item>> items;

	// This structure makes it possible to access all flags, or map/stat flags separately.
	union {
		struct
		{
			uint16_t mapflags;
			uint16_t statflags;
		};
		uint32_t flags;
	};
};

inline uint16_t Tile::getMapFlags() const
{
	return mapflags;
}
inline uint16_t Tile::getStatFlags() const
{
	return statflags;
}