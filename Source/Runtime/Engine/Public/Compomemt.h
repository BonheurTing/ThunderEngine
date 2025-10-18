#pragma once
#include "Mesh.h"
#include "NameHandle.h"
#include "Guid.h"
#include "Vector.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
#include <atomic>

namespace Thunder
{
	class IComponent : public GameObject
	{
	public:
		IComponent() = default;
		virtual ~IComponent() = default;

		// Component identification
		virtual NameHandle GetComponentName() const = 0;

		// Dependency collection for resource streaming
		virtual void GetDependencies(TArray<TGuid>& OutDependencies) const {}

		// JSON serialization
		virtual void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& Writer) const {}
		virtual void DeserializeJson(const rapidjson::Value& JsonValue) {}

		// Asynchronous resource loading
		virtual void AsyncLoad() {}
		virtual bool IsLoaded() const { return true; }
		virtual void OnLoaded() {}

		// Factory method for creating components by name
		static IComponent* CreateComponentByName(const NameHandle& componentName);
	};

	class PrimitiveComponent : public IComponent
	{
	public:
		PrimitiveComponent() = default;
		virtual ~PrimitiveComponent() = default;

		NameHandle GetComponentName() const override { return "PrimitiveComponent"; }
	};

	class StaticMeshComponent : public PrimitiveComponent
	{
	public:
		StaticMeshComponent() = default;
		~StaticMeshComponent() override = default;

		NameHandle GetComponentName() const override { return "StaticMeshComponent"; }

		// Dependency collection
		void GetDependencies(TArray<TGuid>& outDependencies) const override;

		// JSON serialization
		void SerializeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
		void DeserializeJson(const rapidjson::Value& jsonValue) override;

		// Asynchronous resource loading
		void AsyncLoad() override;
		bool IsLoaded() const override;
		void OnLoaded() override;

		// Synchronous resource loading (blocking)
		void SyncLoad();

		// Mesh and material management
		void SetMesh(StaticMesh* inMesh) { Mesh = inMesh; }
		StaticMesh* GetMesh() const { return Mesh; }

		void SetOverrideMaterials(const TArray<IMaterial*>& inMaterials) { OverrideMaterials = inMaterials; }
		const TArray<IMaterial*>& GetOverrideMaterials() const { return OverrideMaterials; }

	private:
		StaticMesh* Mesh { nullptr };
		TArray<IMaterial*> OverrideMaterials {};

		// Loading state
		std::atomic<bool> Loaded { false };

		// GUIDs for serialization
		TGuid MeshGuid {};
		TArray<TGuid> MaterialGuids {};
	};

	// Transform component for entity positioning
	class TransformComponent : public IComponent
	{
	public:
		TransformComponent() = default;
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
