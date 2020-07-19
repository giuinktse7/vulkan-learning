#include "quad_tree.h"

#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

#include "debug.h"

using namespace quadtree;
using namespace std;

// uint32_t has 32 bits: +- get 16 bits each. 4 least sig.
// is used within a chunk. This gives (16 - 4) / 2 levels in the tree.
constexpr uint32_t LEVELS_IN_QUAD_TREE = (16 - 4) / 2;

// The implementation assumes a map in the range [-65535, 65535]

Node::Node(NodeType nodeType)
    : nodeType(nodeType)
{
  cout << "Construct node" << endl;
}

Node::~Node()
{

  if (isLeaf())
  {
    cout << "Destruct child node" << endl;
    std::destroy(children.begin(), children.end());
  }
  else
  {
    cout << "Destruct node" << endl;
    std::destroy(nodes.begin(), nodes.end());
  }
}

bool Node::isLeaf() const
{
  return nodeType == NodeType::Leaf;
}

bool Node::isRoot() const
{
  return nodeType == NodeType::Root;
}

TileLocation &Floor::getTileLocation(int x, int y)
{
  return locations[(x & 3) * 4 + (y & 3)];
}

TileLocation &Floor::getTileLocation(uint32_t index)
{
  DEBUG_ASSERT(index < MAP_LAYERS, "Index '" + std::to_string(index) + "' is larger than MAP_LAYERS (=" + std::to_string(MAP_LAYERS) + ").");

  return locations[index];
}

Floor &Node::createFloor(int x, int y, int z)
{
  DEBUG_ASSERT(isLeaf(), "Only leaf nodes can create a floor.");

  if (!children[z])
  {
    children[z] = std::make_unique<Floor>(x, y, z);
  }

  return *children[z];
}

Floor::Floor(int x, int y, int z)
{
  cout << "Floor()" << endl;
  // Since the map is chunked into 4x4, the first two bytes do not matter here
  // for x and y
  x &= ~3;
  y &= ~3;

  /* The tiles are stored as (01 means x = 0, y = 1):
      00, 01, 02, 03,
      10, 11, 12, 13,
      20, 21, 22, 23,
      30, 31, 32, 33,
  */
  for (int i = 0; i < MAP_TREE_CHILDREN_COUNT; ++i)
  {
    locations[i].position.x = x + (i >> 2);
    locations[i].position.y = y + (i & 3);
    locations[i].position.z = z;
  }
}

Floor::~Floor()
{
  cout << "~Floor()" << endl;
}

Floor *Node::getFloor(uint32_t z) const
{
  DEBUG_ASSERT(isLeaf(), "Only leaves contain floors.");
  return this->children[z].get();
}

Node *Node::getLeafUnsafe(int x, int y) const
{
  Node *node = const_cast<Node *>(this);

  uint32_t currentX = x;
  uint32_t currentY = y;

  while (node)
  {
    if (node->isLeaf())
    {
      return node;
    }

    uint32_t index = ((currentX & 0xC000) >> 14) | ((currentY & 0xC000) >> 12);

    std::unique_ptr<Node> &child = node->nodes[index];
    if (!child)
    {
      return nullptr;
    }

    node = child.get();
    currentX <<= 2;
    currentY <<= 2;
  }
}

Node &Node::getLeaf(int x, int y)
{
  Node *node = getLeafUnsafe(x, y);
  if (!node)
  {
    std::stringstream s;
    s << "There is no leaf for position { x=" << x << ", y=" << y << " }.";
    ABORT_PROGRAM(s.str());
  }

  return *node;
}

Node &Node::getLeafWithCreate(int x, int y)
{
  Node *node = this;
  uint32_t currentX = x;
  uint32_t currentY = y;

  uint8_t level = 6;

  while (node)
  {
    /*  The index is given by the bytes YYXX.
        XX is given by the two MSB in currentX, and YY by the two MSB in currentY.
    */
    uint32_t index = ((currentX & 0xC000) >> 14) | ((currentY & 0xC000) >> 12);
    // cout << "index: " << index << endl;

    std::unique_ptr<Node> &child = node->nodes[index];
    if (child)
    {
      if (child->isLeaf())
      {
        return *child;
      }
    }
    else
    {
      if (level == 0)
      {
        child = make_unique<Node>(NodeType::Leaf);
        return *child;
      }
      else
      {
        child = make_unique<Node>(NodeType::Node);
      }
    }

    node = child.get();
    currentX <<= 2;
    currentY <<= 2;
    --level;
  }
}