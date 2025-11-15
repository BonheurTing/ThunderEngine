#pragma optimize("", off)
#include "Entity.h"
#include "GameModule.h"
#include "rapidjson/document.h"

namespace Thunder
{
	Entity::Entity(Scene* inScene, GameObject* inOuter)
		: OwnerScene(inScene), GameObject(inOuter)
	{
		Transform = new (TMemory::Malloc<TransformComponent>()) TransformComponent(this);
		NameHandle componentName = Transform->GetComponentName();
		ComponentTable[componentName] = Transform;
	}

	Entity::~Entity()
	{
		GameModule::UnregisterTickable(this);

		if (Transform)
		{
			TMemory::Destroy(Transform);
		}

		for (IComponent* component : Components)
		{
			TMemory::Destroy(component);
		}
		Components.clear();
		ComponentTable.clear();

		for (Entity* child : Children)
		{
			delete child;
		}
		Children.clear();
		Status.store(LoadingStatus::Idle, std::memory_order_release);
	}

	IComponent* Entity::GetComponentByName(const NameHandle& componentName) const
	{
		auto it = ComponentTable.find(componentName);
		if (it != ComponentTable.end())
		{
			return it->second;
		}
		return nullptr;
	}

	void Entity::GetAllHierarchyComponents(TList<IComponent*>& outComponents) const
	{
		// Add this entity's Transform component
		outComponents.push_back(Transform);

		// Add this entity's other components
		for (IComponent* comp : Components)
		{
			outComponents.push_back(comp);
		}

		// Recursively add children's components
		for (Entity* child : Children)
		{
			child->GetAllHierarchyComponents(outComponents);
		}
	}

	void Entity::AddChild(Entity* child)
	{
		if (child && child->Owner != this)
		{
			if (child->Owner)
			{
				child->Owner->RemoveChild(child);
			}
			child->Owner = this;
			Children.push_back(child);
		}
	}

	void Entity::RemoveChild(Entity* child)
	{
		auto it = std::find(Children.begin(), Children.end(), child);
		if (it != Children.end())
		{
			(*it)->Owner = nullptr;
			Children.erase(it);
		}
	}

	void Entity::GetDependencies(TList<TGuid>& outDependencies) const
	{
		for (IComponent* component : Components)
		{
			component->GetDependencies(outDependencies);
		}

		// Recursively collect dependencies from children
		for (Entity* child : Children)
		{
			child->GetDependencies(outDependencies);
		}
	}

	void Entity::SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const
	{
		writer.StartObject();

		// Serialize entity name
		writer.Key("EntityName");
		writer.String(EntityName.c_str());

		// Serialize components
		writer.Key("Components");
		writer.StartObject();
		TAssert( Transform != nullptr);
		writer.Key(Transform->GetComponentName().c_str());
		Transform->SerializeJson(writer);
		for (IComponent* component : Components)
		{
			writer.Key(component->GetComponentName().c_str());
			component->SerializeJson(writer);
		}
		writer.EndObject();

		// Serialize children
		if (!Children.empty())
		{
			writer.Key("Children");
			writer.StartArray();
			for (Entity* child : Children)
			{
				child->SerializeJson(writer);
			}
			writer.EndArray();
		}

		writer.EndObject();
	}

	void Entity::DeserializeJson(const rapidjson::Value& jsonValue)
	{
		if (!jsonValue.IsObject())
		{
			return;
		}

		// Deserialize entity name
		if (jsonValue.HasMember("EntityName") && jsonValue["EntityName"].IsString())
		{
			EntityName = NameHandle(jsonValue["EntityName"].GetString());
		}

		// Deserialize components
		if (jsonValue.HasMember("Components") && jsonValue["Components"].IsObject())
		{
			const rapidjson::Value& componentsObj = jsonValue["Components"];
			bool isFirstComponent = true;
			for (auto it = componentsObj.MemberBegin(); it != componentsObj.MemberEnd(); ++it)
			{
				const char* componentTypeName = it->name.GetString();
				const rapidjson::Value& ComponentData = it->value;

				// Create component based on type name
				IComponent* newComponent = IComponent::CreateComponentByName(this, NameHandle(componentTypeName));
				if (newComponent)
				{
					newComponent->DeserializeJson(ComponentData);

					// First component should be TransformComponent
					if (isFirstComponent)
					{
						isFirstComponent = false;
						TransformComponent* transformComp = dynamic_cast<TransformComponent*>(newComponent);
						TAssert( transformComp != nullptr && Transform != nullptr );

						// Delete existing Transform and assign the deserialized one
						TMemory::Destroy(Transform);
						Transform = transformComp;
						ComponentTable[Transform->GetComponentName()] = Transform;
					}
					else
					{
						// All other components go into the Components array
						Components.push_back(newComponent);
						ComponentTable[newComponent->GetComponentName()] = newComponent;
					}
				}
			}
		}

		// Deserialize children
		if (jsonValue.HasMember("Children") && jsonValue["Children"].IsArray())
		{
			const rapidjson::Value& childrenArray = jsonValue["Children"];
			for (rapidjson::SizeType i = 0; i < childrenArray.Size(); ++i)
			{
				Entity* child = new Entity(OwnerScene, this);
				child->DeserializeJson(childrenArray[i]);
				AddChild(child);
			}
		}
	}

	void Entity::Load()
	{
		auto curStatus = Status.load(std::memory_order_acquire);
		if (curStatus == LoadingStatus::Loading || curStatus == LoadingStatus::Loaded)
		{
			return;
		}
		Status.store(LoadingStatus::Loading, std::memory_order_release);
		for (auto& comp : Components)
		{
			comp->LoadAsync();
		}
		for (auto child : Children)
		{
			child->Load();
		}
	}

	void Entity::OnLoaded()
	{
		Status.store(LoadingStatus::Loaded, std::memory_order_release);
		GameModule::RegisterTickable(this);
	}

	void Entity::Tick()
	{
		//
	}
}
