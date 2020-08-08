#pragma once

#include <unordered_map>
#include <vector>

#include "entity.h"
#include "../debug.h"

#include <type_traits>

class IComponentArray
{
public:
	virtual void entityDestroyed(Entity entity) = 0;
	virtual ~IComponentArray() = default;
};

template <typename T>
class ComponentArray : public IComponentArray
{
	using EntityId = uint32_t;

public:
	ComponentArray()
	{
		std::cout << "ComponentArray()" << std::endl;
	}
	~ComponentArray() override
	{
		std::cout << "~ComponentArray()" << std::endl;
	}

	void addComponent(Entity entity, T &component);
	void addComponent(Entity entity, T &&component);

	void removeComponent(Entity entity)
	{
		if (entityIndex.find(entity) != entityIndex.end())
		{
			size_t componentCount = components.size();
			size_t removedIndex = entityIndex[entity];
			if (std::is_move_assignable<T>::value && std::is_move_constructible<T>::value)
			{
				// components[removedIndex] = std::move(components.back());
			}
			else
			{
				// components[removedIndex] = components.back();
			}

			components.pop_back();
			entityIndex.erase(entity);
		}
	}

	void entityDestroyed(Entity entity) override
	{
		removeComponent(entity);
	}

	bool hasComponent(Entity entity) const
	{
		return entityIndex.find(entity) != entityIndex.end();
	}

	T *getComponent(Entity entity)
	{
		// DEBUG_ASSERT(entityIndex.find(entity) != entityIndex.end(), "Entity " + std::to_string(entity.id) + " does not have a " + std::string(typeid(T).name()) + " component.");

		return &components.at(entityIndex.at(entity));
	}

private:
	std::vector<T> components;
	std::unordered_map<Entity, size_t> entityIndex;
};

template <typename T>
inline void ComponentArray<T>::addComponent(Entity entity, T &component)
{
	DEBUG_ASSERT(entityIndex.find(entity) == entityIndex.end(), "The entity " + std::to_string(entity.id) + " already has a component of type " + typeid(T).name());

	components.push_back(component);
	entityIndex.emplace(entity, components.size() - 1);
}

template <typename T>
inline void ComponentArray<T>::addComponent(Entity entity, T &&component)
{
	DEBUG_ASSERT(entityIndex.find(entity) == entityIndex.end(), "The entity " + std::to_string(entity.id) + " already has a component of type " + typeid(T).name());

	components.push_back(component);
	entityIndex.emplace(entity, components.size() - 1);
}
