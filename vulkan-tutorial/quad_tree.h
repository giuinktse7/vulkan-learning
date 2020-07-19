#pragma once

#include <stdint.h>
#include <array>
#include <memory>

#include "tile_location.h"
#include "const.h"

class MapIterator;
class Map;

enum class NodeType
{
	Root,
	Node,
	Leaf
};

class Floor
{
public:
	Floor(int x, int y, int z);
	~Floor();

	Floor(const Floor &) = delete;
	Floor &operator=(const Floor &) = delete;

	TileLocation &getTileLocation(int x, int y);
	TileLocation &getTileLocation(uint32_t index);

private:
	// x, y locations
	TileLocation locations[MAP_TREE_CHILDREN_COUNT];
};

namespace quadtree
{
	class Node
	{
	public:
		Node(NodeType nodeType);
		~Node();

		Node(const Node &) = delete;
		Node &operator=(const Node &) = delete;

		// Get a leaf node. Creates the leaf node if it does not already exist.
		Node &getLeafWithCreate(int x, int y);
		Node &getLeaf(int x, int y);
		Node *getLeafUnsafe(int x, int y) const;

		Floor &createFloor(int x, int y, int z);
		Floor *getFloor(uint32_t z) const;

		bool isLeaf() const;
		bool isRoot() const;

		friend class Map;
		friend class MapIterator;

	protected:
		NodeType nodeType = NodeType::Root;
		union {
			std::array<std::unique_ptr<Node>, MAP_TREE_CHILDREN_COUNT> nodes{};
			std::array<std::unique_ptr<Floor>, MAP_TREE_CHILDREN_COUNT> children;
		};
	};
}; // namespace quadtree