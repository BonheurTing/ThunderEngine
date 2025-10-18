#include "Compomemt.h"
#include "Guid.h"
#include "ResourceModule.h"
#include "Concurrent/TaskScheduler.h"
#include "Assertion.h"
#include "Scene.h"

namespace Thunder
{
	// Component factory
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
		else if (componentName == "PrimitiveComponent")
		{
			return new PrimitiveComponent();
		}
		// Add more component types as needed
		return nullptr;
	}

	// StaticMeshComponent implementation
	void StaticMeshComponent::GetDependencies(TArray<TGuid>& outDependencies) const
	{
		// Add mesh GUID
		if (Mesh)
		{
			outDependencies.push_back(Mesh->GetGUID());
		}
		else if (MeshGuid.IsValid())
		{
			outDependencies.push_back(MeshGuid);
		}

		// Add material GUIDs
		for (IMaterial* material : OverrideMaterials)
		{
			if (material)
			{
				outDependencies.push_back(material->GetGUID());
			}
		}

		// Also add from MaterialGuids
		for (const TGuid& matGuid : MaterialGuids)
		{
			if (matGuid.IsValid())
			{
				outDependencies.push_back(matGuid);
			}
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

	// StaticMeshComponent loading implementation
	void StaticMeshComponent::SyncLoad()
	{
		TArray<TGuid> guidList;
		GetDependencies(guidList);

		if (guidList.empty())
		{
			// No dependencies, mark as loaded
			OnLoaded();
			return;
		}

		// Load all dependencies synchronously using ParallelFor
		uint32 guidNum = static_cast<uint32>(guidList.size());

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

		GAsyncWorkers->ParallelFor([guidList, callback](uint32 bundleBegin, uint32 bundleSize, uint32 bundleId) mutable
		{
			for (uint32 index = bundleBegin; index < bundleBegin + bundleSize; ++index)
			{
				auto guid = guidList[index];
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
