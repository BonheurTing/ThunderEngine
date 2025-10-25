#pragma optimize("", off)
#include "Compomemt.h"
#include "GameModule.h"
#include "Guid.h"
#include "ResourceModule.h"
#include "Concurrent/TaskScheduler.h"
#include "Assertion.h"
#include "Scene.h"

namespace Thunder
{
	IComponent::~IComponent()
	{
		GameModule::UnregisterTickable(this);
	}

	bool IComponent::IsLoaded() const
	{
		return Status.load(std::memory_order_acquire) == LoadingStatus::Loaded;
	}

	void IComponent::OnLoaded()
	{
		Status.store(LoadingStatus::Loaded, std::memory_order_release);
		GameModule::RegisterTickable(this);
	}

	void IComponent::Tick()
	{
		// Base implementation does nothing
		// Derived classes can override this method
	}

	// todo Component factory
	IComponent* IComponent::CreateComponentByName(const NameHandle& componentName)
	{
		if (componentName == "StaticMeshComponent")
		{
			return new StaticMeshComponent();
		}
		else if (componentName == "TransformComponent")
		{
			return new TransformComponent();
		}
		// Add more component types as needed
		return nullptr;
	}

	// StaticMeshComponent implementation
	void StaticMeshComponent::GetDependencies(TList<TGuid>& outDependencies) const
	{
		// Add mesh GUID
		if (MeshGuid.IsValid())
		{
			outDependencies.push_back(MeshGuid);
		}

		// Add material GUIDs
		for (const TGuid& matGuid : MaterialGuids)
		{
			TAssert(matGuid.IsValid());
			outDependencies.push_back(matGuid);
		}
	}

	void StaticMeshComponent::SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const
	{
		writer.StartObject();

		// Serialize mesh GUID
		writer.Key("Mesh");
		if (Mesh)
		{
			char guidStr[64];
			snprintf(guidStr, sizeof(guidStr), "%08X-%08X-%08X-%08X",
				Mesh->GetGUID().A, Mesh->GetGUID().B, Mesh->GetGUID().C, Mesh->GetGUID().D);
			writer.String(guidStr);
		}
		else
		{
			writer.Null();
		}

		// Serialize override materials
		if (!OverrideMaterials.empty())
		{
			for (size_t i = 0; i < OverrideMaterials.size(); ++i)
			{
				char keyName[64];
				snprintf(keyName, sizeof(keyName), "Material%zu", i);
				writer.Key(keyName);

				if (OverrideMaterials[i])
				{
					char GuidStr[64];
					snprintf(GuidStr, sizeof(GuidStr), "%08X-%08X-%08X-%08X",
						OverrideMaterials[i]->GetGUID().A, OverrideMaterials[i]->GetGUID().B,
						OverrideMaterials[i]->GetGUID().C, OverrideMaterials[i]->GetGUID().D);
					writer.String(GuidStr);
				}
				else
				{
					writer.Null();
				}
			}
		}

		writer.EndObject();
	}

	void StaticMeshComponent::DeserializeJson(const rapidjson::Value& jsonValue)
	{
		if (!jsonValue.IsObject())
		{
			return;
		}

		// Deserialize mesh GUID
		if (jsonValue.HasMember("Mesh") && jsonValue["Mesh"].IsString())
		{
			const char* guidStr = jsonValue["Mesh"].GetString();
			// Parse GUID string (format: "XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX")
			uint32 A, B, C, D;
			if (sscanf(guidStr, "%08X-%08X-%08X-%08X", &A, &B, &C, &D) == 4)
			{
				MeshGuid = TGuid(A, B, C, D);
			}
		}

		// Deserialize override materials
		MaterialGuids.clear();
		for (size_t i = 0; i < 100; ++i) // Assume max 100 materials
		{
			char keyName[64];
			snprintf(keyName, sizeof(keyName), "Material%zu", i);

			if (jsonValue.HasMember(keyName) && jsonValue[keyName].IsString())
			{
				const char* guidStr = jsonValue[keyName].GetString();
				uint32 A, B, C, D;
				if (sscanf(guidStr, "%08X-%08X-%08X-%08X", &A, &B, &C, &D) == 4)
				{
					MaterialGuids.push_back(TGuid(A, B, C, D));
				}
			}
			else
			{
				break; // No more materials
			}
		}
	}

	void StaticMeshComponent::LoadAsync()
	{
		auto curStatus = Status.load(std::memory_order_acquire);
		if (curStatus == LoadingStatus::Loading || curStatus == LoadingStatus::Loaded)
		{
			return;
		}
		Status.store(LoadingStatus::Loading, std::memory_order_release);

		if (!MeshGuid.IsValid() && MaterialGuids.empty())
		{
			// No dependencies, mark as loaded
			OnLoaded();
			return;
		}

		TWeakObjectPtr<StaticMeshComponent> componentPtr = this;
		GAsyncWorkers->PushTask([meshGuid = MeshGuid, guidList = MaterialGuids, componentPtr]()
		{
			TStrongObjectPtr<GameResource> meshRef= ResourceModule::LoadSync(meshGuid);

			TArray<TStrongObjectPtr<GameResource>> materialRefList;
			materialRefList.reserve(guidList.size());
			for (uint32 i = 0; i < static_cast<uint32>(guidList.size()); ++i)
			{
				materialRefList[i] = ResourceModule::LoadSync(guidList[i]);
			}
			
			GGameScheduler->PushTask([componentPtr, meshRef, materialRefList]()
			{
				if (componentPtr)
				{
					componentPtr->Mesh = static_cast<StaticMesh*>(meshRef.Get());
					componentPtr->OverrideMaterials.reserve(materialRefList.size());
					for (uint32 i = 0; i < static_cast<uint32>(materialRefList.size()); ++i)
					{
						componentPtr->OverrideMaterials[i] = static_cast<IMaterial*>(materialRefList[i].Get());
					}
					componentPtr->OnLoaded();
				}
			});
		});
	}

	void StaticMeshComponent::OnLoaded()
	{
		// Call parent OnLoaded to mark as loaded and register tickable.
		IComponent::OnLoaded();
		LOG("StaticMeshComponent loaded successfully");
	}

	// TransformComponent implementation
	void TransformComponent::SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const
	{
		writer.StartObject();

		writer.Key("Position");
		writer.StartArray();
		writer.Double(Position.X);
		writer.Double(Position.Y);
		writer.Double(Position.Z);
		writer.EndArray();

		writer.Key("Rotation");
		writer.StartArray();
		writer.Double(Rotation.X);
		writer.Double(Rotation.Y);
		writer.Double(Rotation.Z);
		writer.EndArray();

		writer.Key("Scale");
		writer.StartArray();
		writer.Double(Scale.X);
		writer.Double(Scale.Y);
		writer.Double(Scale.Z);
		writer.EndArray();

		writer.EndObject();
	}

	void TransformComponent::DeserializeJson(const rapidjson::Value& jsonValue)
	{
		if (!jsonValue.IsObject())
		{
			return;
		}

		// Deserialize position
		if (jsonValue.HasMember("Position") && jsonValue["Position"].IsArray())
		{
			const rapidjson::Value& posArray = jsonValue["Position"];
			if (posArray.Size() >= 3)
			{
				Position.X = static_cast<float>(posArray[0].GetDouble());
				Position.Y = static_cast<float>(posArray[1].GetDouble());
				Position.Z = static_cast<float>(posArray[2].GetDouble());
			}
		}

		// Deserialize rotation
		if (jsonValue.HasMember("Rotation") && jsonValue["Rotation"].IsArray())
		{
			const rapidjson::Value& rotArray = jsonValue["Rotation"];
			if (rotArray.Size() >= 3)
			{
				Rotation.X = static_cast<float>(rotArray[0].GetDouble());
				Rotation.Y = static_cast<float>(rotArray[1].GetDouble());
				Rotation.Z = static_cast<float>(rotArray[2].GetDouble());
			}
		}

		// Deserialize scale
		if (jsonValue.HasMember("Scale") && jsonValue["Scale"].IsArray())
		{
			const rapidjson::Value& scaleArray = jsonValue["Scale"];
			if (scaleArray.Size() >= 3)
			{
				Scale.X = static_cast<float>(scaleArray[0].GetDouble());
				Scale.Y = static_cast<float>(scaleArray[1].GetDouble());
				Scale.Z = static_cast<float>(scaleArray[2].GetDouble());
			}
		}
	}
}
