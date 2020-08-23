#pragma once

#include <memory>
#include <optional>

#include "tile_location.h"
#include "item.h"

class MapView;
class TileLocation;

class Tile
{
public:
	Tile(TileLocation &location);
	~Tile();

	Tile(const Tile &) = delete;
	Tile &operator=(const Tile &) = delete;
	Tile(Tile &&other) noexcept;
	Tile &operator=(Tile &&other) noexcept;

	Tile deepCopy() const;

	/*
		Deselect entire tile
	*/
	void deselectAll();
	void deselectTopItem();
	void selectTopItem();

	bool hasSelection() const;

	Item *getTopItem() const;
	bool hasTopItem() const;
	Item *getGround() const;

	void addItem(Item &&item);
	void removeItem(size_t index);
	void removeGround();

	bool isEmpty() const;

	int getTopElevation() const;

	const std::vector<Item> &getItems() const
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

	uint16_t getMapFlags() const;
	uint16_t getStatFlags() const;

	void setLocation(TileLocation &location);

	void selectItemAtIndex(size_t index);
	void deselectItemAtIndex(size_t index);
	void selectAll();

	const Position getPosition() const;

	long getX() const;
	long getY() const;
	long getZ() const;

	bool topItemSelected() const;

	bool allSelected() const;

private:
	friend class MapView;
	Tile(Position position);

	Position position;
	std::unique_ptr<Item> ground;
	std::vector<Item> items;

	size_t selectionCount;

	// This structure makes it possible to access all flags, or map/stat flags separately.
	union
	{
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