#pragma once
#include "Mesh.h"

namespace Thunder
{
	class IComponent : public GameObject
	{
	public:
		IComponent();
		virtual ~IComponent();
	};

	class StaticMeshComponent : public IComponent
	{
	public:
	private:
		StaticMesh* Mesh { nullptr };
		TArray<IMaterial*> OverrideMaterials {};
	};
}
