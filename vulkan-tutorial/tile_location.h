#pragma once

#include "position.h"

class TileLocation
{
	TileLocation();

public:
	~TileLocation();

	TileLocation(const TileLocation &) = delete;
	TileLocation &operator=(const TileLocation &) = delete;

	friend class Floor;

protected:
	Position position;
};