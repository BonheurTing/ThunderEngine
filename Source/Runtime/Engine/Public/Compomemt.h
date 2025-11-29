#pragma once
// #include "Entity.h"
#include "Mesh.h"
#include "NameHandle.h"
#include "Guid.h"
#include "Vector.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
#include "Concurrent/TaskScheduler.h"
#include "Concurrent/ConcurrentBase.h"

namespace Thunder
{
	/**
	 * Generic load event callback template
	 * Used for notifying when asynchronous resource loading is complete
	 */
	template <typename T>
	class TLoadEvent : public IOnCompleted
	{
	public:
		TLoadEvent(T* inLoadItem) : LoadItem(inLoadItem) {}

		void OnCompleted() override
		{
			if (LoadItem)
			{
				GGameScheduler->PushTask([item = LoadItem]()
				{
					item->OnLoaded();
				});
			}
		}

	private:
		T* LoadItem { nullptr };
	};

	class IComponent : public GameObject, public ITickable
	{
	public:
		IComponent(class Entity* inOwner) : Owner(inOwner) {}
		~IComponent() override;

		// Component identification
		virtual NameHandle GetComponentName() const = 0;

		// Dependency collection for resource streaming
		virtual void GetDependencies(TList<TGuid>& OutDependencies) const {}

		// JSON serialization
		virtual void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& Writer) const = 0;
		virtual void DeserializeJson(const rapidjson::Value& JsonValue) = 0;

		virtual void LoadAsync() {}
		bool IsLoaded() const;
		virtual void OnLoaded();
		void Tick() override;

		// Factory method for creating components by name
		static IComponent* CreateComponentByName(Entity* inOwner, const NameHandle& componentName);

	protected:
		// Load & Lifecycle
		std::atomic<LoadingStatus> Status { LoadingStatus::Idle };
		Entity* Owner { nullptr };
	};

	class PrimitiveComponent : public IComponent
	{
	public:
		PrimitiveComponent(Entity* inOwner) : IComponent(inOwner) {}
		virtual ~PrimitiveComponent() = default;

		NameHandle GetComponentName() const override { return "PrimitiveComponent"; }

		virtual const TArray<IMaterial*>& GetMaterials() const = 0;
	};

	class StaticMeshComponent : public PrimitiveComponent
	{
	public:
		StaticMeshComponent(Entity* inOwner) : PrimitiveComponent(inOwner) {}
		~StaticMeshComponent() override = default;

		NameHandle GetComponentName() const override { return "StaticMeshComponent"; }

		// Dependency collection
		void GetDependencies(TList<TGuid>& outDependencies) const override;

		// JSON serialization
		void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
		void DeserializeJson(const rapidjson::Value& jsonValue) override;

		// resource loading
		void LoadAsync() override;
		void OnLoaded() override;

		// Mesh and material management
		void SetMesh(StaticMesh* inMesh) { Mesh = inMesh; }
		StaticMesh* GetMesh() const { return Mesh; }

		void SetOverrideMaterials(const TArray<IMaterial*>& inMaterials) { OverrideMaterials = inMaterials; }
		const TArray<IMaterial*>& GetOverrideMaterials() const { return OverrideMaterials; }

		const TArray<IMaterial*>& GetMaterials() const override { return OverrideMaterials; }

	private:
		StaticMesh* Mesh { nullptr };
		TArray<IMaterial*> OverrideMaterials {};

		// GUIDs for serialization
		TGuid MeshGuid {};
		TArray<TGuid> MaterialGuids {};

		// render
		class StaticMeshSceneProxy* SceneProxy { nullptr };
	};

	// Transform component for entity positioning
	class TransformComponent : public IComponent
	{
	public:
		TransformComponent(Entity* inOwner) : IComponent(inOwner) {}
		~TransformComponent() override = default;

		NameHandle GetComponentName() const override { return "TransformComponent"; }

		// JSON serialization
		void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
		void DeserializeJson(const rapidjson::Value& jsonValue) override;

		// Transform accessors
		void SetPosition(const TVector3f& inPosition) { Position = inPosition; }
		const TVector3f& GetPosition() const { return Position; }

		void SetRotation(const TVector3f& inRotation) { Rotation = inRotation; }
		const TVector3f& GetRotation() const { return Rotation; }

		void SetScale(const TVector3f& inScale) { Scale = inScale; }
		const TVector3f& GetScale() const { return Scale; }

	private:
		TVector3f Position { 0.0f, 0.0f, 0.0f };
		TVector3f Rotation { 0.0f, 0.0f, 0.0f };
		TVector3f Scale { 1.0f, 1.0f, 1.0f };
	};
}
