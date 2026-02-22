#pragma optimize("", off)
#include "Compomemt.h"
#include "GameModule.h"
#include "Guid.h"
#include "PackageModule.h"
#include "Concurrent/TaskScheduler.h"
#include "Assertion.h"
#include "IRenderer.h"
#include "PrimitiveSceneProxy.h"
#include "Scene.h"
#include "MathUtilities.h"

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
		else if (componentName == "CameraComponent")
		{
			return new CameraComponent(inOwner);
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

		int depCounter = MeshGuid.IsValid() ? 1 : 0;
		depCounter += static_cast<int>(MaterialGuids.size());
		if (depCounter == 0)
		{
			// No dependencies, mark as loaded
			OnLoaded();
			return;
		}

		TLoadEvent<StaticMeshComponent>* loadEvent = new (TMemory::Malloc<TLoadEvent<StaticMeshComponent>>()) TLoadEvent(this);
		loadEvent->Promise(depCounter);
		if (MeshGuid.IsValid())
		{
			PackageModule::LoadAsync(MeshGuid, [loadEvent](){ loadEvent->Notify(); });
		}
		for (auto matGuid : MaterialGuids | std::views::values)
		{
			PackageModule::LoadAsync(matGuid, [loadEvent](){ loadEvent->Notify(); });
		}
		
	}

	void StaticMeshComponent::OnLoaded()
	{
		OverrideMaterials.clear();
		//check is in game thread
		GameResource* expectedLoaded = PackageModule::TryGetLoadedResource(MeshGuid);
		if (expectedLoaded && expectedLoaded->GetResourceType() == ETempGameResourceReflective::StaticMesh)
		{
			Mesh = static_cast<StaticMesh*>(expectedLoaded);
		}
		for (const auto& [fst, snd] : MaterialGuids)
		{
			expectedLoaded = PackageModule::TryGetLoadedResource(snd);
			if (expectedLoaded && expectedLoaded->GetResourceType() == ETempGameResourceReflective::Material)
			{ 
				OverrideMaterials[fst] = static_cast<IMaterial*>(expectedLoaded);
			}
		}

		// Call parent OnLoaded to mark as loaded and register tickable.
		IComponent::OnLoaded();
		LOG("------------ StaticMeshComponent loaded successfully");

		TMatrix44f transform = TMatrix44f::Identity();
		if (Owner)
		{
			transform = Owner->GetTransform();
		}
		SceneProxy = new (TMemory::Malloc<StaticMeshSceneProxy>()) StaticMeshSceneProxy(this, transform);
		Owner->GetScene()->GetRenderer()->RegisterSceneInfo(SceneProxy->GetSceneInfo());
		Owner->GetScene()->GetRenderer()->UpdatePrimitiveUniformBuffer_GameThread(SceneProxy->GetSceneInfo());
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

		UpdateTransform();
		MarkPrimitiveUniformBufferDirty();
		OnLoaded();
	}

	void TransformComponent::SetPosition(const TVector3f& inPosition)
	{
		Position = inPosition;
		UpdateTransform();
		MarkPrimitiveUniformBufferDirty();
	}

	void TransformComponent::SetRotation(const TVector3f& inRotation)
	{
		Rotation = inRotation;
		UpdateTransform();
		MarkPrimitiveUniformBufferDirty();
	}

	void TransformComponent::SetScale(const TVector3f& inScale)
	{
		Scale = inScale;
		UpdateTransform();
		MarkPrimitiveUniformBufferDirty();
	}

	void TransformComponent::MarkPrimitiveUniformBufferDirty()
	{
		// todo : deduplicate
		TList<IComponent*> compList;
		Owner->GetAllHierarchyComponents(compList);
		for (auto& comp : compList)
		{
			// todo : reflect
			if (auto stMeshComp = dynamic_cast<StaticMeshComponent*>(comp))
			{
				StaticMeshSceneProxy* sceneProxy = stMeshComp->GetSceneProxy();
				if (!sceneProxy) [[unlikely]]
				{
					TAssertf(false, "Lack of valid scene proxy for component \"%s\" in entity  \"%s\".", stMeshComp->GetComponentName(), Owner->GetEntityName());
					continue;
				}

				GRenderScheduler->PushTask([transform = Transform, sceneProxy]()
				{
					sceneProxy->UpdateTransform(transform);
				});
				Owner->GetScene()->GetRenderer()->UpdatePrimitiveUniformBuffer_GameThread(sceneProxy->GetSceneInfo());
			}
		}
	}

	// CameraComponent implementation
	void CameraComponent::SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const
	{
		writer.StartObject();
		writer.Key("FovY");        writer.Double(FovY);
		writer.Key("Aspect");      writer.Double(Aspect);
		writer.Key("NearPlane");   writer.Double(NearPlane);
		writer.Key("FarPlane");    writer.Double(FarPlane);
		writer.Key("InfiniteFar"); writer.Bool(bInfiniteFar);
		writer.EndObject();
	}

	void CameraComponent::DeserializeJson(const rapidjson::Value& jsonValue)
	{
		if (!jsonValue.IsObject())
		{
			return;
		}

		if (jsonValue.HasMember("FovY") && jsonValue["FovY"].IsNumber())
		{
			FovY = static_cast<float>(jsonValue["FovY"].GetDouble());
		}
		if (jsonValue.HasMember("Aspect") && jsonValue["Aspect"].IsNumber())
		{
			Aspect = static_cast<float>(jsonValue["Aspect"].GetDouble());
		}
		if (jsonValue.HasMember("NearPlane") && jsonValue["NearPlane"].IsNumber())
		{
			NearPlane = static_cast<float>(jsonValue["NearPlane"].GetDouble());
		}
		if (jsonValue.HasMember("FarPlane") && jsonValue["FarPlane"].IsNumber())
		{
			FarPlane = static_cast<float>(jsonValue["FarPlane"].GetDouble());
		}
		if (jsonValue.HasMember("InfiniteFar") && jsonValue["InfiniteFar"].IsBool())
		{
			bInfiniteFar = jsonValue["InfiniteFar"].GetBool();
		}

		OnLoaded();
	}

	void CameraComponent::SetFovY(float inFovYDegrees)
	{
		TAssertf(inFovYDegrees > 0.0f && inFovYDegrees < 180.0f,
			"CameraComponent::SetFovY: FovY must be in range (0, 180), got %.2f.", inFovYDegrees);
		FovY = inFovYDegrees;
	}

	void CameraComponent::SetAspect(float inAspect)
	{
		TAssertf(inAspect > 0.0f,
			"CameraComponent::SetAspect: Aspect must be positive, got %.4f.", inAspect);
		Aspect = inAspect;
	}

	void CameraComponent::SetNearPlane(float inNear)
	{
		TAssertf(inNear > 0.0f,
			"CameraComponent::SetNearPlane: NearPlane must be > 0, got %.4f.", inNear);
		NearPlane = inNear;
	}

	void CameraComponent::SetFarPlane(float inFar)
	{
		TAssertf(inFar > NearPlane,
			"CameraComponent::SetFarPlane: FarPlane (%.4f) must be greater than NearPlane (%.4f).", inFar, NearPlane);
		FarPlane = inFar;
	}

	void CameraComponent::SetInfiniteFar(bool bInInfiniteFar)
	{
		bInfiniteFar = bInInfiniteFar;
	}

	TMatrix44f CameraComponent::GetProjectionMatrix() const
	{
		return Math::PerspectiveProjectionMatrix(
			FovY * DEG_TO_RAD,
			Aspect,
			NearPlane,
			FarPlane,
			bInfiniteFar);
	}

	void CameraComponent::Tick()
	{
		TMatrix44f viewMatrix = Owner->GetTransform().Inverse();
		TMatrix44f projectionMatrix = GetProjectionMatrix();
		TMatrix44f vpMatrix = viewMatrix * projectionMatrix;

		GRenderScheduler->PushTask([this, vpMatrix]()
		{
			Owner->GetScene()->GetRenderer()->GetFrameGraph()->SetViewProjectionMatrix(OwnerView, vpMatrix);
		});
	}

	void TransformComponent::UpdateTransform()
	{
		// Left-handed coordinate system (UE convention): X=Forward, Y=Right, Z=Up
		// Rotation stored as degrees: X=Pitch(around Y), Y=Yaw(around Z), Z=Roll(around X)
		// Application order: Yaw(Z) -> Pitch(Y) -> Roll(X)
		// R = RotZ * RotY * RotX (row-vector convention: p' = p * R)

		constexpr float DegToRad = DEG_TO_RAD;
		const float pitchRad = Rotation.X * DegToRad;
		const float yawRad   = Rotation.Y * DegToRad;
		const float rollRad  = Rotation.Z * DegToRad;

		const float cp = std::cos(pitchRad);
		const float sp = std::sin(pitchRad);
		const float cy = std::cos(yawRad);
		const float sy = std::sin(yawRad);
		const float cr = std::cos(rollRad);
		const float sr = std::sin(rollRad);

		// Left-handed rotation matrices:
		//   RotZ(yaw):  [ cy  sy  0 ]    RotY(pitch): [ cp  0 -sp ]    RotX(roll): [ 1   0   0 ]
		//               [-sy  cy  0 ]                  [  0  1   0 ]                [ 0  cr  sr ]
		//               [  0   0  1 ]                  [ sp  0  cp ]                [ 0 -sr  cr ]
		//
		// R = RotZ * RotY * RotX:
		//   [ cy*cp,               sy*cr + cy*sp*sr,    sy*sr - cy*sp*cr ]
		//   [-sy*cp,               cy*cr - sy*sp*sr,    cy*sr + sy*sp*cr ]
		//   [ sp,                 -cp*sr,               cp*cr            ]

		const float r00 =  cy * cp;
		const float r01 =  sy * cr + cy * sp * sr;
		const float r02 =  sy * sr - cy * sp * cr;

		const float r10 = -sy * cp;
		const float r11 =  cy * cr - sy * sp * sr;
		const float r12 =  cy * sr + sy * sp * cr;

		const float r20 =  sp;
		const float r21 = -cp * sr;
		const float r22 =  cp * cr;

		// Combined Transform = S * R * T (row-vector convention: p' = p * S * R * T)
		// Row i of rotation is scaled by Scale[i], translation in row 3.
		Transform = TMatrix44f(
			r00 * Scale.X, r01 * Scale.X, r02 * Scale.X, 0.0f,
			r10 * Scale.Y, r11 * Scale.Y, r12 * Scale.Y, 0.0f,
			r20 * Scale.Z, r21 * Scale.Z, r22 * Scale.Z, 0.0f,
			Position.X,    Position.Y,     Position.Z,     1.0f);
	}
}
