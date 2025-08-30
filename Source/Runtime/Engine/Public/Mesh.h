#pragma once
#include "Container.h"
#include "GameObject.h"
#include "Material.h"
#include "Vector.h"
#include "Container/ReflectiveContainer.h"

namespace Thunder
{
	struct AABB
	{
		TVector3f Min { TVector3f(0.f, 0.f, 0.f) };
		TVector3f Max { TVector3f(0.f, 0.f, 0.f) };

		AABB() = default;
		AABB(const TVector3f& min, const TVector3f& max)
			: Min(min), Max(max) {}
		~AABB() = default;

		void Serialize(MemoryWriter& archive) const
		{
			archive << Min;
			archive << Max;
		}
		void DeSerialize(MemoryReader& archive)
		{
			archive >> Min;
			archive >> Max;
		}
	};
	
	class IMesh : public GameResource
	{
	public:
		IMesh(GameObject* inOuter = nullptr, ETempGameResourceReflective inType = ETempGameResourceReflective::Unknown)
			: GameResource(inOuter, inType) {}

		~IMesh() override = default;
	};

	struct SubMesh : public RefCountedObject
	{
		TReflectiveContainerRef Vertices { nullptr };
		TReflectiveContainerRef Indices { nullptr };
		AABB BoundingBox {};
	};
	using TSubMeshRef = TRefCountPtr<SubMesh>;

	class StaticMesh : public IMesh
	{
	public:
		StaticMesh(GameObject* inOuter = nullptr) : IMesh(inOuter, ETempGameResourceReflective::StaticMesh) {}
		~StaticMesh() override = default;

		void Serialize(MemoryWriter& archive) override;
		void DeSerialize(MemoryReader& archive) override;

	public:
		TArray<TSubMeshRef> SubMeshes {};
		TArray<IMaterial*> DefaultMaterials {};

		// UProperty()
		// TArray<TGuid> DefaultMaterialGuids{};
	};
}
