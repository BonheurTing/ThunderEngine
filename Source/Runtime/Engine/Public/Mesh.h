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
	};
	
	class IMesh : public GameResource
	{
	public:
		IMesh() {}
		virtual ~IMesh() {}
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
		StaticMesh() {}
		virtual ~StaticMesh() {}
	public:
		TArray<TSubMeshRef> SubMeshes {};
		TArray<IMaterial*> DefaultMaterials {};

		// UProperty()
		// TArray<TGuid> DefaultMaterialGuids{};
	};
}
