#pragma once

#include <stdint.h>

class ECS;

struct Entity
{
	uint32_t id;

	// Entity &operator=(Entity other) = default;

private:
	friend class ECS;
	Entity(uint32_t id) : id(id)
	{
	}
};

inline bool operator==(const Entity &a, const Entity &b)
{
	return a.id == b.id;
}

namespace std
{
	template <>
	struct hash<Entity>
	{
		std::size_t operator()(const Entity &entity) const noexcept
		{
			return std::hash<uint32_t>{}(entity.id);
		}
	};
} // namespace std