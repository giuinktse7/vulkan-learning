#pragma once

#include <memory>
#include <unordered_map>
#include <iostream>

#include "item.h"
#include "tile.h"
#include "tile_location.h"
#include "quad_tree.h"
#include "position.h"

#include <stack>

class Map
{
private:
	class Iterator
	{
	public:
		Iterator();
		~Iterator();

		Iterator begin() const
		{
			return *this;
		}

		Iterator *nextFromLeaf();

		Iterator end()
		{
			Iterator iterator;
			return iterator;
		}

		// Mark the iterator as finished
		void finish();

		void push(quadtree::Node &node)
		{
			stack.push({node});
		}

		TileLocation *operator*();
		TileLocation *operator->();
		Iterator &operator++();

		bool operator==(const Iterator &other) const
		{
			return other.floorIndex == floorIndex &&
						 other.tileIndex == tileIndex &&
						 other.value == value;
		}

		bool operator!=(const Iterator &other) const
		{
			return !(other == *this);
		}

		struct NodeIndex
		{
			uint32_t cursor = 0;
			quadtree::Node &node;

			bool operator==(const NodeIndex &other)
			{
				return other.cursor == cursor && &other.node == &node;
			}
			bool operator==(NodeIndex &other)
			{
				return other.cursor == cursor && &other.node == &node;
			}

			NodeIndex(quadtree::Node &node) : cursor(0), node(node) {}
		};

		friend class Map;

	private:
		const bool isEnd() const
		{
			return value == nullptr;
		}

		std::stack<NodeIndex> stack;
		uint32_t tileIndex = 0;
		uint32_t floorIndex = 0;
		TileLocation *value = nullptr;
	};

public:
	Map();

	void printTiles();

	Iterator begin();
	Iterator end();

	Tile &getOrCreateTile(int x, int y, int z);
	Tile &getOrCreateTile(Position &pos);
	TileLocation *getTileLocation(int x, int y, int z) const;

private:
	quadtree::Node root;
};
