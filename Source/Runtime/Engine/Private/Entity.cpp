#include "Entity.h"
#include "rapidjson/document.h"

namespace Thunder
{
	Entity::Entity(GameObject* inOuter)
		: GameObject(inOuter)
	{
	}

	Entity::~Entity()
	{
		for (IComponent* component : Components)
		{
			delete component;
		}
		Components.clear();
		ComponentTable.clear();

		for (Entity* child : Children)
		{
			delete child;
		}
		Children.clear();
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

	void Entity::AddChild(Entity* child)
	{
		if (child && child->Parent != this)
		{
			if (child->Parent)
			{
				child->Parent->RemoveChild(child);
			}
			child->Parent = this;
			Children.push_back(child);
		}
	}

	void Entity::RemoveChild(Entity* child)
	{
		auto it = std::find(Children.begin(), Children.end(), child);
		if (it != Children.end())
		{
			(*it)->Parent = nullptr;
			Children.erase(it);
		}
	}

	void Entity::AsyncLoad()
	{
		// Load all components
		for (IComponent* component : Components)
		{
			component->AsyncLoad();
		}

		// Load all children recursively
		for (Entity* child : Children)
		{
			child->AsyncLoad();
		}
	}

	void Entity::GetDependencies(TArray<TGuid>& outDependencies) const
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
			for (auto it = componentsObj.MemberBegin(); it != componentsObj.MemberEnd(); ++it)
			{
				const char* componentTypeName = it->name.GetString();
				const rapidjson::Value& ComponentData = it->value;

				// Create component based on type name
				IComponent* newComponent = IComponent::CreateComponentByName(NameHandle(componentTypeName));
				if (newComponent)
				{
					newComponent->DeserializeJson(ComponentData);
					Components.push_back(newComponent);
					ComponentTable[newComponent->GetComponentName()] = newComponent;
				}
			}
		}

		// Deserialize children
		if (jsonValue.HasMember("Children") && jsonValue["Children"].IsArray())
		{
			const rapidjson::Value& childrenArray = jsonValue["Children"];
			for (rapidjson::SizeType i = 0; i < childrenArray.Size(); ++i)
			{
				Entity* child = new Entity(this);
				child->DeserializeJson(childrenArray[i]);
				AddChild(child);
			}
		}
	}
}
