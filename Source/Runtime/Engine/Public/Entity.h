#pragma once
#include "GameObject.h"
#include "Compomemt.h"
#include "NameHandle.h"
#include "Container.h"
//#include "Scene.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

namespace Thunder
{
	class TransformComponent;
	/**
	 * Entity represents a game object in the scene hierarchy
	 * Similar to Actor in Unreal Engine
	 */
	class Entity : public GameObject, public ITickable
	{
	public:
		ENGINE_API Entity(class Scene* inScene, GameObject* inOuter = nullptr);
		ENGINE_API virtual ~Entity();

		// Component management
		template<typename T>
		T* AddComponent();

		template<typename T>
		T* GetComponent() const;

		template<typename T>
		TArray<IComponent*> GetComponentByClass() const;

		ENGINE_API IComponent* GetComponentByName(const NameHandle& componentName) const;

		template<typename T>
		void RemoveComponent();
		
		ENGINE_API void GetAllHierarchyComponents(TList<IComponent*>& outComponents) const;

		// Hierarchy management
		ENGINE_API void AddChild(Entity* child);
		ENGINE_API void RemoveChild(Entity* child);
		ENGINE_API Entity* GetOwner() const { return Owner; }
		ENGINE_API Scene* GetScene() const { return OwnerScene; }
		ENGINE_API const TArray<Entity*>& GetChildren() const { return Children; }

		// Dependency collection for resource streaming
		ENGINE_API void GetDependencies(TList<TGuid>& outDependencies) const;

		// JSON serialization for scene persistence
		ENGINE_API void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
		ENGINE_API void DeserializeJson(const rapidjson::Value& jsonValue);

		// Entity identification
		ENGINE_API void SetEntityName(const NameHandle& inName) { EntityName = inName; }
		ENGINE_API const NameHandle& GetEntityName() const { return EntityName; }
		ENGINE_API TransformComponent* GetTransformComponent() const;
		ENGINE_API TMatrix44f GetTransform() const;

		ENGINE_API void Load();
		ENGINE_API void OnLoaded();
		ENGINE_API void Tick() override;
	private:
		NameHandle EntityName;
		TransformComponent* Transform { nullptr };
		TArray<IComponent*> Components; //uporperty - gc flush
		TMap<NameHandle, IComponent*> ComponentTable;

		// Hierarchy
		Scene* OwnerScene { nullptr };
		Entity* Owner { nullptr }; //uporperty
		TArray<Entity*> Children; //uporperty

		// Load & Lifecycle
		std::atomic<LoadingStatus> Status { LoadingStatus::Idle };
	};

	template<typename T>
	T* Entity::AddComponent()
	{
		static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from IComponent");
		static_assert(!std::is_same<T, TransformComponent>::value, "Cannot add TransformComponent via AddComponent - use GetTransform() instead");

		T* newComponent = new (TMemory::Malloc<T>()) T(this);
		Components.push_back(newComponent);

		NameHandle componentName = newComponent->GetComponentName();
		ComponentTable[componentName] = newComponent;

		return newComponent;
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from IComponent");

		for (auto it = Components.begin(); it != Components.end(); ++it)
		{
			T* castedComponent = static_cast<T*>(*it);
			if (castedComponent)
			{
				NameHandle componentName = castedComponent->GetComponentName();
				ComponentTable.erase(componentName);
				TMemory::Destroy(castedComponent);
				Components.erase(it);
				return;
			}
		}
	}

	template<typename T>
	TArray<IComponent*> Entity::GetComponentByClass() const
	{
		static_assert(std::is_base_of<IComponent, T>::value, "T must be derived from IComponent");

		TList<IComponent*> temp;
		for (IComponent* component : Components)
		{
			if (T* castedComponent = dynamic_cast<T*>(component))
			{
				temp.push_back(castedComponent);
			}
		}
		TArray<IComponent*> ret(temp.begin(), temp.end());
		return ret;
	}
}
