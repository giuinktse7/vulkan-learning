#pragma once

#include <stdint.h>
#include <ostream>

#include "const.h"

class MapView;
struct WorldPosition;
struct MapPosition;

template <typename T>
struct BasePosition
{
	T x;
	T y;
};

struct Position : public BasePosition<long>
{
	int z;

	Position &operator+=(const Position &rhs)
	{
		this->x += rhs.x;
		this->y += rhs.y;
		this->z += rhs.z;
		return *this;
	}

	Position &operator-=(const Position &rhs)
	{
		this->x -= rhs.x;
		this->y -= rhs.y;
		this->z -= rhs.z;
		return *this;
	}

	Position operator-(const Position &rhs)
	{
		*this -= rhs;
		return *this;
	}

	template <int I>
	auto get() const
	{
		if constexpr (I == 0)
			return x;
		else if constexpr (I == 1)
			return y;
		else if constexpr (I == 2)
			return z;
	}
};

struct ScreenPosition : public BasePosition<double>
{
	WorldPosition worldPos(const MapView &mapView);
	MapPosition mapPos(const MapView &mapView);
	Position toPos(const MapView &mapView);
};

struct WorldPosition : public BasePosition<double>
{
	MapPosition mapPos();
};

struct MapPosition : public BasePosition<long>
{
	WorldPosition worldPos();

	Position floor(int floor);
};

enum GameDirection : uint8_t
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

template <typename T>
inline bool operator==(const BasePosition<T> &pos1, const BasePosition<T> &pos2)
{
	return pos1.x == pos2.x && pos1.y == pos2.y;
}

template <typename T>
inline bool operator!=(const BasePosition<T> &pos1, const BasePosition<T> &pos2)
{
	return !(pos1 == pos2);
}

namespace std
{
	template <>
	struct tuple_size<Position> : std::integral_constant<size_t, 3>
	{
	};

	template <size_t I>
	class std::tuple_element<I, Position>
	{
	public:
		using type = decltype(declval<Position>().get<I>());
	};

} // namespace std
