#pragma once

#include <stdint.h>
#include <array>
#include <memory>

#include "tile_location.h"

enum class NodeType
{
	Root,
	Node,
	Leaf
};

constexpr uint16_t MAP_TREE_CHILDREN_COUNT = 16;

class Floor
{
public:
	Floor(int x, int y, int z);
	~Floor();

	Floor(const Floor &) = delete;
	Floor &operator=(const Floor &) = delete;

	TileLocation &getTileLocation(int x, int y);

private:
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

		Floor &createFloor(int x, int y, int z);

		bool isLeaf();
		bool isRoot();

	private:
		NodeType nodeType = NodeType::Root;
		union
		{
			std::array<std::unique_ptr<Node>, MAP_TREE_CHILDREN_COUNT> nodes{};
			std::array<std::unique_ptr<Floor>, MAP_TREE_CHILDREN_COUNT> children;
		};
	};
}; // namespace quadtree