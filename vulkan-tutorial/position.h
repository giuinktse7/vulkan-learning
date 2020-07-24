#pragma once

#include <stdint.h>
#include <ostream>

enum Direction : uint8_t
{
	DIRECTION_NORTH = 0,
	DIRECTION_EAST = 1,
	DIRECTION_SOUTH = 2,
	DIRECTION_WEST = 3,

	DIRECTION_DIAGONAL_MASK = 4,
	DIRECTION_SOUTHWEST = DIRECTION_DIAGONAL_MASK | 0,
	DIRECTION_SOUTHEAST = DIRECTION_DIAGONAL_MASK | 1,
	DIRECTION_NORTHWEST = DIRECTION_DIAGONAL_MASK | 2,
	DIRECTION_NORTHEAST = DIRECTION_DIAGONAL_MASK | 3,

	DIRECTION_LAST = DIRECTION_NORTHEAST,
	DIRECTION_NONE = 8,
};

struct Position
{
	uint32_t x, y, z;
};

inline bool operator==(const Position &pos1, const Position &pos2)
{
	return pos1.x == pos2.x && pos1.y == pos2.y && pos1.z == pos2.z;
}

inline bool operator!=(const Position &pos1, const Position &pos2)
{
	return !(pos1 == pos2);
}

inline std::ostream &operator<<(std::ostream &os, const Position &pos)
{
	os << pos.x << ':' << pos.y << ':' << pos.z;
	return os;
}