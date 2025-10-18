#pragma once
#include "GameObject.h"
#include "Compomemt.h"
#include "NameHandle.h"
#include "Container.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

namespace Thunder
{
	/**
	 * Entity represents a game object in the scene hierarchy
	 * Similar to Actor in Unreal Engine
	 */
	class ENGINE_API Entity : public GameObject
	{
	public:
		Entity(GameObject* inOuter = nullptr);
		virtual ~Entity();

		// Component management
		template<typename T>
		T* AddComponent();

		template<typename T>
		T* GetComponent() const;

		IComponent* GetComponentByName(const NameHandle& componentName) const;

		template<typename T>
		void RemoveComponent();

		const TArray<IComponent*>& GetAllComponents() const { return Components; }

		// Hierarchy management
		void AddChild(Entity* child);
		void RemoveChild(Entity* child);
		Entity* GetParent() const { return Parent; }
		const TArray<Entity*>& GetChildren() const { return Children; }

		// Dependency collection for resource streaming
		void GetDependencies(TArray<TGuid>& outDependencies) const;

		// JSON serialization for scene persistence
		void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		void DeserializeJson(const rapidjson::Value& jsonValue);

		// Asynchronous resource loading
		void AsyncLoad();

		// Entity identification
		void SetEntityName(const NameHandle& inName) { EntityName = inName; }
		const NameHandle& GetEntityName() const { return EntityName; }

	private:
		NameHandle EntityName;
		TArray<IComponent*> Components; //uporperty - gc flush
		TMap<NameHandle, IComponent*> ComponentTable;

		// Hierarchy
		Entity* Parent { nullptr }; //uporperty 
		TArray<Entity*> Children; //uporperty 
	};

	template<typename T>
	T* Entity::AddComponent()
	{
		static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from IComponent");

		T* newComponent = new T();
		Components.push_back(newComponent);

		NameHandle componentName = newComponent->GetComponentName();
		ComponentTable[componentName] = newComponent;

		return newComponent;
	}

	template<typename T>
	T* Entity::GetComponent() const
	{
		static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from IComponent");

		for (IComponent* component : Components)
		{
			T* castedComponent = dynamic_cast<T*>(component);
			if (castedComponent)
			{
				return castedComponent;
			}
		}
		return nullptr;
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from IComponent");

		for (auto it = Components.begin(); it != Components.end(); ++it)
		{
			T* castedComponent = dynamic_cast<T*>(*it);
			if (castedComponent)
			{
				NameHandle componentName = castedComponent->GetComponentName();
				ComponentTable.erase(componentName);
				delete castedComponent;
				Components.erase(it);
				return;
			}
		}
	}
}
