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
		virtual RenderMesh* CreateResource_GameThread() = 0;
		void SetResource(RenderMesh* resource);
		void ReleaseResource();
		void InitResource();

	private:
		RenderMeshRef MeshResource {};
	};

	class StaticMesh : public IMesh
	{
	public:
		StaticMesh(GameObject* inOuter = nullptr)
			: IMesh(inOuter, ETempGameResourceReflective::StaticMesh)
		{
		}
		~StaticMesh() override;

		void Serialize(MemoryWriter& archive) override;
		void DeSerialize(MemoryReader& archive) override;

		void AddMaterial(IMaterial* material);
		const TArray<IMaterial*>& GetMaterials() { return DefaultMaterials; }

		RenderMesh* CreateResource_GameThread() override;

	public:
		TArray<SubMesh*> SubMeshes {}; //Serialize
		TArray<IMaterial*> DefaultMaterials {}; // Dependencies guid //12.28todo: material slot
	};
}
