#pragma once

#include <memory>
#include <unordered_map>
#include <iostream>
#include <string>

#include "debug.h"

#include "item.h"
#include "tile.h"
#include "tile_location.h"
#include "quad_tree.h"
#include "position.h"

#include "town.h"

#include "version.h"

#include <stack>
#include <optional>

class MapIterator
{
public:
	MapIterator();
	~MapIterator();

	MapIterator begin() const
	{
		return *this;
	}

	MapIterator *nextFromLeaf();

	MapIterator end()
	{
		MapIterator iterator;
		iterator.finish();
		return iterator;
	}

	// Mark the iterator as finished
	void finish();

	void emplace(quadtree::Node *node);

	TileLocation *operator*();
	TileLocation *operator->();
	MapIterator &operator++();

	bool operator==(const MapIterator &other) const
	{
		return other.floorIndex == floorIndex &&
					 other.tileIndex == tileIndex &&
					 other.value == value;
	}

	bool operator!=(const MapIterator &other) const
	{
		return !(other == *this);
	}

	struct NodeIndex
	{
		uint32_t cursor = 0;
		quadtree::Node *node;

		NodeIndex(quadtree::Node *node) : cursor(0), node(node) {}

		// bool operator==(const NodeIndex &other)
		// {
		// 	return other.cursor == cursor && &other.node == &node;
		// }
		// bool operator==(NodeIndex &other)
		// {
		// 	return other.cursor == cursor && &other.node == &node;
		// }
	};

	friend class Map;

private:
	std::stack<NodeIndex> stack{};
	uint32_t tileIndex = 0;
	uint32_t floorIndex = 0;
	TileLocation *value = nullptr;
};

class Map
{
public:
	Map();

	MapIterator begin();
	MapIterator end();

	/*
		Replace the tile at the given tile's location. Returns the old tile if one
		was present.
	*/
	std::unique_ptr<Tile> replaceTile(Tile &&tile);
	Tile &getOrCreateTile(int x, int y, int z);
	Tile &getOrCreateTile(const Position &pos);
	TileLocation *getTileLocation(int x, int y, int z) const;
	TileLocation *getTileLocation(const Position &pos) const;
	Tile *getTile(const Position pos) const;
	void removeTile(const Position pos);

	bool isTileEmpty(const Position pos) const;

	MapVersion getMapVersion();
	std::string &getDescription();

	void createItemAt(Position pos, uint16_t id);

	uint16_t getWidth() const;
	uint16_t getHeight() const;

	Towns &getTowns()
	{
		return towns;
	}

	quadtree::Node *getLeafUnsafe(int x, int y);

private:
	Towns towns;
	MapVersion mapVersion;
	std::string description;

	uint16_t width, height;

	quadtree::Node root;
};

inline uint16_t Map::getWidth() const
{
	return width;
}
inline uint16_t Map::getHeight() const
{
	return height;
}