#include "Compomemt.h"
#include "Guid.h"
#include "ResourceModule.h"
#include "Concurrent/TaskScheduler.h"
#include "Assertion.h"
#include "Scene.h"

namespace Thunder
{
	// Component factory
	IComponent* IComponent::CreateComponentByName(const NameHandle& ComponentName)
	{
		if (ComponentName == "StaticMeshComponent")
		{
			return new StaticMeshComponent();
		}
		else if (ComponentName == "TransformComponent")
		{
			return new TransformComponent();
		}
		else if (ComponentName == "PrimitiveComponent")
		{
			return new PrimitiveComponent();
		}
		// Add more component types as needed
		return nullptr;
	}

	// StaticMeshComponent implementation
	void StaticMeshComponent::GetDependencies(TArray<TGuid>& OutDependencies) const
	{
		// Add mesh GUID
		if (Mesh)
		{
			OutDependencies.push_back(Mesh->GetGUID());
		}
		else if (MeshGuid.IsValid())
		{
			OutDependencies.push_back(MeshGuid);
		}

		// Add material GUIDs
		for (IMaterial* Material : OverrideMaterials)
		{
			if (Material)
			{
				OutDependencies.push_back(Material->GetGUID());
			}
		}

		// Also add from MaterialGuids
		for (const TGuid& MatGuid : MaterialGuids)
		{
			if (MatGuid.IsValid())
			{
				OutDependencies.push_back(MatGuid);
			}
		}
	}

	void StaticMeshComponent::SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& Writer) const
	{
		Writer.StartObject();

		// Serialize mesh GUID
		Writer.Key("Mesh");
		if (Mesh)
		{
			char GuidStr[64];
			snprintf(GuidStr, sizeof(GuidStr), "%08X-%08X-%08X-%08X",
				Mesh->GetGUID().A, Mesh->GetGUID().B, Mesh->GetGUID().C, Mesh->GetGUID().D);
			Writer.String(GuidStr);
		}
		else
		{
			Writer.Null();
		}

		// Serialize override materials
		if (!OverrideMaterials.empty())
		{
			for (size_t i = 0; i < OverrideMaterials.size(); ++i)
			{
				char KeyName[64];
				snprintf(KeyName, sizeof(KeyName), "Material%zu", i);
				Writer.Key(KeyName);

				if (OverrideMaterials[i])
				{
					char GuidStr[64];
					snprintf(GuidStr, sizeof(GuidStr), "%08X-%08X-%08X-%08X",
						OverrideMaterials[i]->GetGUID().A, OverrideMaterials[i]->GetGUID().B,
						OverrideMaterials[i]->GetGUID().C, OverrideMaterials[i]->GetGUID().D);
					Writer.String(GuidStr);
				}
				else
				{
					Writer.Null();
				}
			}
		}

		Writer.EndObject();
	}

	void StaticMeshComponent::DeserializeJson(const rapidjson::Value& JsonValue)
	{
		if (!JsonValue.IsObject())
		{
			return;
		}

		// Deserialize mesh GUID
		if (JsonValue.HasMember("Mesh") && JsonValue["Mesh"].IsString())
		{
			const char* GuidStr = JsonValue["Mesh"].GetString();
			// Parse GUID string (format: "XXXXXXXX-XXXXXXXX-XXXXXXXX-XXXXXXXX")
			uint32 A, B, C, D;
			if (sscanf(GuidStr, "%08X-%08X-%08X-%08X", &A, &B, &C, &D) == 4)
			{
				MeshGuid = TGuid(A, B, C, D);
			}
		}

		// Deserialize override materials
		MaterialGuids.clear();
		for (size_t i = 0; i < 100; ++i) // Assume max 100 materials
		{
			char KeyName[64];
			snprintf(KeyName, sizeof(KeyName), "Material%zu", i);

			if (JsonValue.HasMember(KeyName) && JsonValue[KeyName].IsString())
			{
				const char* GuidStr = JsonValue[KeyName].GetString();
				uint32 A, B, C, D;
				if (sscanf(GuidStr, "%08X-%08X-%08X-%08X", &A, &B, &C, &D) == 4)
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

	// StaticMeshComponent loading implementation
	void StaticMeshComponent::SyncLoad()
	{
		TArray<TGuid> GuidList;
		GetDependencies(GuidList);

		if (GuidList.empty())
		{
			// No dependencies, mark as loaded
			OnLoaded();
			return;
		}

		// Load all dependencies synchronously using ParallelFor
		uint32 guidNum = static_cast<uint32>(GuidList.size());

		// Create a simple counter-based completion tracker
		/*class LoadEvent : public IOnCompleted
		{
		public:
			LoadEvent(StaticMeshComponent* InComponent) : Component(InComponent) {}
			void OnCompleted() override
			{
				if (Component)
				{
					Component->OnLoaded();
				}
			}
		private:
			StaticMeshComponent* Component;
		};*/

		auto* callback = new (TMemory::Malloc<OnSceneLoaded<StaticMeshComponent>>()) OnSceneLoaded(this);
		callback->Promise(guidNum);

		uint32 taskBundleSize = guidNum / GAsyncWorkers->GetNumThreads();
		if (taskBundleSize == 0) taskBundleSize = 1;

		GAsyncWorkers->ParallelFor([GuidList, callback](uint32 bundleBegin, uint32 bundleSize, uint32 bundleId) mutable
		{
			for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
			{
				auto guid = GuidList[index];
				ResourceModule::LoadSync(guid);
				callback->Notify();
			}
		}, guidNum, taskBundleSize);

		TMemory::Destroy(callback);
	}

	void StaticMeshComponent::AsyncLoad()
	{
		GAsyncWorkers->PushTask([this]()
		{
			SyncLoad();
		});
	}

	bool StaticMeshComponent::IsLoaded() const
	{
		return Loaded.load(std::memory_order_acquire);
	}

	void StaticMeshComponent::OnLoaded()
	{
		// Verify all resources are loaded
		TAssert(ResourceModule::IsLoaded(MeshGuid));
		Mesh = static_cast<StaticMesh*>(ResourceModule::LoadSync(MeshGuid));

		int slot = 0;
		OverrideMaterials.clear();
		for (const TGuid& materialGuid : MaterialGuids)
		{
			TAssert(ResourceModule::IsLoaded(materialGuid));
			IMaterial* material = static_cast<IMaterial*>(ResourceModule::LoadSync(materialGuid));
			if (material)
			{
				OverrideMaterials.push_back(material);
			}
			++slot;
		}

		// Mark as loaded
		Loaded.store(true, std::memory_order_release);

		LOG("StaticMeshComponent loaded successfully");
	}

	// TransformComponent implementation
	void TransformComponent::SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& Writer) const
	{
		Writer.StartObject();

		Writer.Key("Position");
		Writer.StartArray();
		Writer.Double(Position.X);
		Writer.Double(Position.Y);
		Writer.Double(Position.Z);
		Writer.EndArray();

		Writer.Key("Rotation");
		Writer.StartArray();
		Writer.Double(Rotation.X);
		Writer.Double(Rotation.Y);
		Writer.Double(Rotation.Z);
		Writer.EndArray();

		Writer.Key("Scale");
		Writer.StartArray();
		Writer.Double(Scale.X);
		Writer.Double(Scale.Y);
		Writer.Double(Scale.Z);
		Writer.EndArray();

		Writer.EndObject();
	}

	void TransformComponent::DeserializeJson(const rapidjson::Value& JsonValue)
	{
		if (!JsonValue.IsObject())
		{
			return;
		}

		// Deserialize position
		if (JsonValue.HasMember("Position") && JsonValue["Position"].IsArray())
		{
			const rapidjson::Value& PosArray = JsonValue["Position"];
			if (PosArray.Size() >= 3)
			{
				Position.X = static_cast<float>(PosArray[0].GetDouble());
				Position.Y = static_cast<float>(PosArray[1].GetDouble());
				Position.Z = static_cast<float>(PosArray[2].GetDouble());
			}
		}

		// Deserialize rotation
		if (JsonValue.HasMember("Rotation") && JsonValue["Rotation"].IsArray())
		{
			const rapidjson::Value& RotArray = JsonValue["Rotation"];
			if (RotArray.Size() >= 3)
			{
				Rotation.X = static_cast<float>(RotArray[0].GetDouble());
				Rotation.Y = static_cast<float>(RotArray[1].GetDouble());
				Rotation.Z = static_cast<float>(RotArray[2].GetDouble());
			}
		}

		// Deserialize scale
		if (JsonValue.HasMember("Scale") && JsonValue["Scale"].IsArray())
		{
			const rapidjson::Value& ScaleArray = JsonValue["Scale"];
			if (ScaleArray.Size() >= 3)
			{
				Scale.X = static_cast<float>(ScaleArray[0].GetDouble());
				Scale.Y = static_cast<float>(ScaleArray[1].GetDouble());
				Scale.Z = static_cast<float>(ScaleArray[2].GetDouble());
			}
		}
	}
}
