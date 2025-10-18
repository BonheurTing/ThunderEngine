#include "Entity.h"
#include "rapidjson/document.h"

namespace Thunder
{
	Entity::Entity(GameObject* InOuter)
		: GameObject(InOuter)
	{
	}

	Entity::~Entity()
	{
		for (IComponent* Component : Components)
		{
			delete Component;
		}
		Components.clear();
		ComponentTable.clear();

		for (Entity* Child : Children)
		{
			delete Child;
		}
		Children.clear();
	}

	IComponent* Entity::GetComponentByName(const NameHandle& ComponentName) const
	{
		auto It = ComponentTable.find(ComponentName);
		if (It != ComponentTable.end())
		{
			return It->second;
		}
		return nullptr;
	}

	void Entity::AddChild(Entity* Child)
	{
		if (Child && Child->Parent != this)
		{
			if (Child->Parent)
			{
				Child->Parent->RemoveChild(Child);
			}
			Child->Parent = this;
			Children.push_back(Child);
		}
	}

	void Entity::RemoveChild(Entity* Child)
	{
		auto It = std::find(Children.begin(), Children.end(), Child);
		if (It != Children.end())
		{
			(*It)->Parent = nullptr;
			Children.erase(It);
		}
	}

	void Entity::AsyncLoad()
	{
		// Load all components
		for (IComponent* Component : Components)
		{
			Component->AsyncLoad();
		}

		// Load all children recursively
		for (Entity* Child : Children)
		{
			Child->AsyncLoad();
		}
	}

	void Entity::GetDependencies(TArray<TGuid>& OutDependencies) const
	{
		for (IComponent* Component : Components)
		{
			Component->GetDependencies(OutDependencies);
		}

		// Recursively collect dependencies from children
		for (Entity* Child : Children)
		{
			Child->GetDependencies(OutDependencies);
		}
	}

	void Entity::SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& Writer) const
	{
		Writer.StartObject();

		// Serialize entity name
		Writer.Key("EntityName");
		Writer.String(EntityName.c_str());

		// Serialize components
		Writer.Key("Components");
		Writer.StartObject();
		for (IComponent* Component : Components)
		{
			Writer.Key(Component->GetComponentName().c_str());
			Component->SerializeJson(Writer);
		}
		Writer.EndObject();

		// Serialize children
		if (!Children.empty())
		{
			Writer.Key("Children");
			Writer.StartArray();
			for (Entity* Child : Children)
			{
				Child->SerializeJson(Writer);
			}
			Writer.EndArray();
		}

		Writer.EndObject();
	}

	void Entity::DeserializeJson(const rapidjson::Value& JsonValue)
	{
		if (!JsonValue.IsObject())
		{
			return;
		}

		// Deserialize entity name
		if (JsonValue.HasMember("EntityName") && JsonValue["EntityName"].IsString())
		{
			EntityName = NameHandle(JsonValue["EntityName"].GetString());
		}

		// Deserialize components
		if (JsonValue.HasMember("Components") && JsonValue["Components"].IsObject())
		{
			const rapidjson::Value& ComponentsObj = JsonValue["Components"];
			for (auto It = ComponentsObj.MemberBegin(); It != ComponentsObj.MemberEnd(); ++It)
			{
				const char* ComponentTypeName = It->name.GetString();
				const rapidjson::Value& ComponentData = It->value;

				// Create component based on type name
				IComponent* NewComponent = IComponent::CreateComponentByName(NameHandle(ComponentTypeName));
				if (NewComponent)
				{
					NewComponent->DeserializeJson(ComponentData);
					Components.push_back(NewComponent);
					ComponentTable[NewComponent->GetComponentName()] = NewComponent;
				}
			}
		}

		// Deserialize children
		if (JsonValue.HasMember("Children") && JsonValue["Children"].IsArray())
		{
			const rapidjson::Value& ChildrenArray = JsonValue["Children"];
			for (rapidjson::SizeType i = 0; i < ChildrenArray.Size(); ++i)
			{
				Entity* Child = new Entity(this);
				Child->DeserializeJson(ChildrenArray[i]);
				AddChild(Child);
			}
		}
	}
}
