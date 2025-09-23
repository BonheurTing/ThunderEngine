#pragma once
#include "Container.h"
#include "GameObject.h"
#include "Material.h"
#include "RenderMesh.h"

namespace Thunder
{
	class IMesh : public GameResource
	{
	public:
		IMesh(GameObject* inOuter = nullptr, ETempGameResourceReflective inType = ETempGameResourceReflective::Unknown)
			: GameResource(inOuter, inType) {}

		~IMesh() override;

		// game object
		void OnResourceLoaded() override;

		// render resource
		virtual class RenderMesh* CreateResource_GameThread() = 0;
		void SetResource(RenderMesh* Resource);
		void ReleaseResource();
		void InitResource();

	private:
		RenderMeshRef MeshResource {};
	};

	class StaticMesh : public IMesh
	{
	public:
		StaticMesh(GameObject* inOuter = nullptr) : IMesh(inOuter, ETempGameResourceReflective::StaticMesh) {}
		~StaticMesh() override = default;

		void Serialize(MemoryWriter& archive) override;
		void DeSerialize(MemoryReader& archive) override;

		RenderMesh* CreateResource_GameThread() override;

	public:
		TArray<TSubMeshRef> SubMeshes {};
		TArray<IMaterial*> DefaultMaterials {};

		// UProperty()
		// TArray<TGuid> DefaultMaterialGuids{};
	};
}
