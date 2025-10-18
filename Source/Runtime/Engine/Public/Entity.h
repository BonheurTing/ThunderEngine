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
		Entity(GameObject* InOuter = nullptr);
		virtual ~Entity();

		// Component management
		template<typename T>
		T* AddComponent();

		template<typename T>
		T* GetComponent() const;

		IComponent* GetComponentByName(const NameHandle& ComponentName) const;

		template<typename T>
		void RemoveComponent();

		const TArray<IComponent*>& GetAllComponents() const { return Components; }

		// Hierarchy management
		void AddChild(Entity* Child);
		void RemoveChild(Entity* Child);
		Entity* GetParent() const { return Parent; }
		const TArray<Entity*>& GetChildren() const { return Children; }

		// Dependency collection for resource streaming
		void GetDependencies(TArray<TGuid>& OutDependencies) const;

		// JSON serialization for scene persistence
		void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& Writer) const;
		void DeserializeJson(const rapidjson::Value& JsonValue);

		// Asynchronous resource loading
		void AsyncLoad();

		// Entity identification
		void SetEntityName(const NameHandle& InName) { EntityName = InName; }
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

		T* NewComponent = new T();
		Components.push_back(NewComponent);

		NameHandle ComponentName = NewComponent->GetComponentName();
		ComponentTable[ComponentName] = NewComponent;

		return NewComponent;
	}

	template<typename T>
	T* Entity::GetComponent() const
	{
		static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from IComponent");

		for (IComponent* Component : Components)
		{
			T* CastedComponent = dynamic_cast<T*>(Component);
			if (CastedComponent)
			{
				return CastedComponent;
			}
		}
		return nullptr;
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from IComponent");

		for (auto It = Components.begin(); It != Components.end(); ++It)
		{
			T* CastedComponent = dynamic_cast<T*>(*It);
			if (CastedComponent)
			{
				NameHandle ComponentName = CastedComponent->GetComponentName();
				ComponentTable.erase(ComponentName);
				delete CastedComponent;
				Components.erase(It);
				return;
			}
		}
	}
}
