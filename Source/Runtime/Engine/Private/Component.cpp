#pragma optimize("", off)
#include "Compomemt.h"
#include "GameModule.h"
#include "Guid.h"
#include "ResourceModule.h"
#include "Concurrent/TaskScheduler.h"
#include "Assertion.h"
#include "IRenderer.h"
#include "PrimitiveSceneProxy.h"
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
	IComponent* IComponent::CreateComponentByName(Entity* inOwner, const NameHandle& componentName)
	{
		if (componentName == "StaticMeshComponent")
		{
			return new StaticMeshComponent(inOwner);
		}
		else if (componentName == "TransformComponent")
		{
			return new TransformComponent(inOwner);
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
		TAssert(MaterialGuids.size() == OverrideMaterials.size());
		for (auto it = MaterialGuids.begin(); it != MaterialGuids.end(); ++it)
		{
			TAssert(it->second.IsValid());
			outDependencies.push_back(it->second);
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
			writer.Key("OverrideMaterials");
			writer.StartObject();
			for (auto it = OverrideMaterials.begin(); it != OverrideMaterials.end(); ++it)
			{
				writer.Key(it->first.c_str());
				char GuidStr[64];
				TGuid guid = it->second->GetGUID();
				snprintf(GuidStr, sizeof(GuidStr), "%08X-%08X-%08X-%08X", guid.A, guid.B, guid.C, guid.D);
				writer.String(GuidStr);
				
			}
			writer.EndObject();
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
		if (jsonValue.HasMember("OverrideMaterials") && jsonValue["OverrideMaterials"].IsObject())
		{
			const rapidjson::Value& materialObj = jsonValue["OverrideMaterials"];
			for (auto it = materialObj.MemberBegin(); it != materialObj.MemberEnd(); ++it)
			{
				const char* guidStr = it->value.GetString();
				uint32 A, B, C, D;
				if (sscanf(guidStr, "%08X-%08X-%08X-%08X", &A, &B, &C, &D) == 4)
				{
					MaterialGuids.emplace(it->name.GetString(), TGuid(A, B, C, D));
				}
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

			TMap<NameHandle, TStrongObjectPtr<GameResource>> materialRefList;
			for (const auto& [fst, snd] : guidList)
			{
				GameResource* res = ResourceModule::LoadSync(snd);
				materialRefList.emplace(fst, res);
			}

			GGameScheduler->PushTask([componentPtr, meshRef, materialRefList]()
			{
				if (componentPtr)
				{
					componentPtr->Mesh = static_cast<StaticMesh*>(meshRef.Get());
					componentPtr->OverrideMaterials.clear();
					for (const auto& [fst, snd] : materialRefList)
					{
						componentPtr->OverrideMaterials.emplace(fst, static_cast<IMaterial*>(snd.Get()));
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
		LOG("------------ StaticMeshComponent loaded successfully");
		// Register StaticMeshSceneProxy.
		if (OverrideMaterials.empty() && Mesh != nullptr) //SimulateTest
		{
			for (auto mat : Mesh->DefaultMaterials)
			{
				OverrideMaterials.emplace(mat->GetResourceName(), mat);
			}
		}

		SceneProxy = new (TMemory::Malloc<StaticMeshSceneProxy>()) StaticMeshSceneProxy(this);
		Owner->GetScene()->GetRenderer()->RegisterSceneProxy(SceneProxy);
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
