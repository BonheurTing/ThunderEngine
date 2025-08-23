#pragma once
#include "Mesh.h"

namespace Thunder
{
	class IComponent : public GameObject
	{
	public:
		IComponent() = default;
		virtual ~IComponent() = default;
	};

	class StaticMeshComponent : public IComponent
	{
	public:
	private:
		StaticMesh* Mesh { nullptr };
		TArray<IMaterial*> OverrideMaterials {};
	};
}
