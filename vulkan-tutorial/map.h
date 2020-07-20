#pragma once

#include <memory>
#include <unordered_map>
#include <iostream>

#include "item.h"
#include "tile.h"
#include "tile_location.h"
#include "quad_tree.h"

class Map;

class MapIterator
{
public:
	MapIterator();
	~MapIterator();

	MapIterator begin() const
	{
		return *this;
	}

	MapIterator end() const
	{
		MapIterator iterator;
		iterator.localI = -1;
		iterator.localZ = -1;
		return iterator;
	}

	void pushNode(quadtree::Node *node)
	{
		stack.emplace_back<MapIterator::NodeIndex>(node);
	}

	TileLocation *operator*();
	TileLocation *operator->();
	MapIterator &operator++();

	bool operator==(const MapIterator &other) const
	{
		// Check for end
		// if (localZ == -1 && localI == -1 && other.localZ == -1 && other.localI == -1)
		// return true;

		return other.localZ == localZ &&
					 other.localI == localI &&
					 other.stack == stack &&
					 other.currentTile == currentTile;
	}

	bool operator!=(const MapIterator &other) const
	{
		return !(other == *this);
	}

	struct NodeIndex
	{
		NodeIndex(quadtree::Node *node) : index(0), node(node) {}
		NodeIndex(const NodeIndex &other) : index(other.index), node(other.node) {}

		int index;
		quadtree::Node *node;

		bool operator==(const NodeIndex &n) const
		{
			return n.node == node && n.index == index;
		}
	};

	friend class Map;

private:
	const bool isEnd() const
	{
		return localI == -1 && localZ == -1 && stack.empty() && currentTile == nullptr;
	}
	std::vector<NodeIndex> stack;
	int localI;
	int localZ;
	TileLocation *currentTile = nullptr;
};

class Map
{
public:
	Map();

	MapIterator begin();
	MapIterator end();

	Tile &
	createTile(int x, int y, int z);
	TileLocation *getTileLocation(int x, int y, int z) const;

private:
	quadtree::Node root;
};
