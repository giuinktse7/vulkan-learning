#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <queue>
#include <memory>
#include <type_traits>

#include "../debug.h"
#include "../util.h"
#include "entity.h"
#include "component_array.h"

class Intellisense
{
public:
	Intellisense() = delete;
	Intellisense(int x){};
};

class ECS;

extern ECS g_ecs;

constexpr size_t MaxEntityComponents = 32;

namespace ecs
{
	using ComponentBitset = std::bitset<MaxEntityComponents>;

	class System
	{
	public:
		virtual void update() = 0;
		void clear()
		{
			entities.clear();
		}

	protected:
		friend class ECS;

		virtual std::vector<const char *> getRequiredComponents() = 0;

		ecs::ComponentBitset componentBitset;
		std::unordered_set<Entity> entities;
	};
} // namespace ecs

class ECS
{
	using ComponentType = uint8_t;

public:
	Entity createEntity();

	template <typename T>
	void registerComponent()
	{
		const char *typeName = typeid(T).name();

		DEBUG_ASSERT(componentTypes.find(typeName) == componentTypes.end(), "The component type " + std::string(typeName) + " has already been registered.");

		componentTypes.insert({typeName, nextComponentType});
		componentArrays[typeName] = std::make_unique<ComponentArray<T>>();

		++nextComponentType;
	}

	template <typename T>
	ecs::System *registerSystem()
	{
		const char *typeName = typeid(T).name();
		DEBUG_ASSERT(systems.find(typeName) == systems.end(), "The system '" + std::string(typeName) + "' is already registered.");

		std::unique_ptr<ecs::System> system;

		// Pointer magic to cast the T (which should be deriving from ecs::System) to a ecs::System
		std::unique_ptr<T> basePointer = std::make_unique<T>();
		ecs::System *tmp = dynamic_cast<ecs::System *>(basePointer.get());
		if (tmp != nullptr)
		{
			basePointer.release();
			system.reset(tmp);
		}

		for (const auto &componentTypeName : system->getRequiredComponents())
		{
			DEBUG_ASSERT(componentTypes.find(componentTypeName) != componentTypes.end(), "The component type '" + std::string(componentTypeName) + "' is not registered.");

			ECS::ComponentType bit = componentTypes[componentTypeName];
			system->componentBitset.set(bit);
		}

		systems[typeName] = std::move(system);
		return systems[typeName].get();
	}

	template <typename T>
	ComponentType getComponentType()
	{
		const char *typeName = typeid(T).name();

		DEBUG_ASSERT(componentTypes.find(typeName) != componentTypes.end(), "The component type '" + std::string(typeName) + "' is not registered.");

		return componentTypes[typeName];
	}

	template <typename T>
	T *getComponent(Entity entity)
	{
		ComponentArray<T> *componentArray = getComponentArray<T>();
		if (componentArray->hasComponent(entity))
		{
			T *component = componentArray->getComponent(entity);
			return component;
		}
		else
		{
			return nullptr;
		}
	}

	template <typename T>
	T &getSystem()
	{
		const char *typeName = typeid(T).name();

		DEBUG_ASSERT(systems.find(typeName) != systems.end(), "There is no registered '" + std::string(typeName) + "' system.");

		ecs::System &system = *systems.at(typeName).get();
		return dynamic_cast<T &>(system);
	}

	template <typename T>
	void addComponent(Entity entity, T component)
	{
		ComponentArray<T> *componentArray = getComponentArray<T>();
		componentArray->addComponent(entity, component);

		setEntityComponentBit<T>(entity);
	}

	template <typename T>
	void removeComponent(Entity entity)
	{
		ComponentArray<T> *array = getComponentArray<T>();
		array->removeComponent(entity);

		unsetEntityComponentBit<T>(entity);
	}

	template <typename T>
	bool systemRequiresComponent(ecs::System &system)
	{
		return system.componentBitset.test(componentTypes.at(typeid(T).name()));
	}

	template <typename T>
	void removeAllComponents()
	{
		ComponentArray<T> *array = getComponentArray<T>();
		array->clear();

		for (const auto &entry : systems)
		{
			ecs::System *system = entry.second.get();
			if (systemRequiresComponent<T>(*system))
			{
				system->clear();
			}
		}
	}

	void
	destroy(Entity entity)
	{
		for (const auto &entry : componentArrays)
		{
			const auto &componentArray = entry.second;
			componentArray->entityDestroyed(entity);
		}

		for (const auto &entry : systems)
		{
			const auto &system = entry.second;
			system->entities.erase(entity);
		}

		entityIdQueue.emplace(entity.id);
		entityComponentBitsets.erase(entity.id);
	}

private:
	uint32_t nextComponentType = 0;
	uint32_t entityCounter = 0;

	std::queue<uint32_t> entityIdQueue;
	std::unordered_map<uint32_t, ecs::ComponentBitset> entityComponentBitsets;

	std::unordered_map<const char *, ComponentType> componentTypes;
	std::unordered_map<const char *, std::unique_ptr<IComponentArray>> componentArrays;

	std::unordered_map<const char *, std::unique_ptr<ecs::System>> systems;

	template <typename T>
	ComponentArray<T> *getComponentArray()
	{
		const char *typeName = typeid(T).name();
		DEBUG_ASSERT(componentTypes.find(typeName) != componentTypes.end(), "The component type '" + std::string(typeName) + "' is not registered.");

		return dynamic_cast<ComponentArray<T> *>(componentArrays.at(typeName).get());
	}

	template <typename T>
	void setEntityComponentBit(Entity entity)
	{
		ECS::ComponentType componentType = getComponentType<T>();
		auto &bitset = entityComponentBitsets.at(entity.id);
		bitset.set(componentType);

		onEntityBitsetChanged(entity, bitset);
	}

	template <typename T>
	void unsetEntityComponentBit(Entity entity)
	{
		ECS::ComponentType componentType = getComponentType<T>();
		auto &bitset = entityComponentBitsets.at(entity.id);
		bitset.reset(componentType);

		onEntityBitsetChanged(entity, bitset);
	}

	void onEntityBitsetChanged(Entity entity, ecs::ComponentBitset bitset)
	{
		for (const auto &entry : systems)
		{
			ecs::System *system = entry.second.get();
			if ((bitset & system->componentBitset) == system->componentBitset)
			{
				system->entities.emplace(entity);
			}
			else
			{
				system->entities.erase(entity);
			}
		}
	}
};