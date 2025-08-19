#pragma once
#include "BasicDefinition.h"
#include "Guid.h"
#include "NameHandle.h"
#include "FileSystem/MemoryArchive.h"

namespace Thunder
{
	class GameObject
	{
	public:
		GameObject() = default;
		virtual ~GameObject() = default;
		_NODISCARD_ GameObject* GetOuter() const { return Outer; }

		virtual void Serialize(MemoryWriter& archive) {}

	private:
		GameObject* Outer { nullptr };
	};

	class GameResource : public GameObject
	{
	public:
		GameResource()
		{
			TGuid::GenerateGUID(Guid);
		}
		_NODISCARD_ TGuid GetGUID() const { return Guid; }
		void AddDependency(const GUID& guid) { Dependencies.push_back(guid); }

	private:
		TGuid Guid {};
		NameHandle ResourceName {}; // 资源的虚拟路径（如 "/Game/Textures/Texture1"）
		TArray<GUID> Dependencies; // 该资源需要的其他资源的guid，加载的时候需要一起加载
	};
}
