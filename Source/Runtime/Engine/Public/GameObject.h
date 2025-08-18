#pragma once
#include "BasicDefinition.h"
#include "NameHandle.h"
#include "FileSystem/MemoryArchive.h"

namespace Thunder
{
	struct GUID
	{
		uint32_t Data1;
		uint16_t Data2;
		uint16_t Data3;
		uint8_t Data4[8];

		GUID() : Data1(0), Data2(0), Data3(0) { memset(Data4, 0, sizeof(Data4)); }
	};

	class GameObject
	{
	public:
		GameObject();
		virtual ~GameObject();
		_NODISCARD_ GameObject* GetOuter() const { return Outer; }

		virtual void Serialize(MemoryWriter& archive){}

	private:
		GameObject* Outer;
	};

	class GameResource : public GameObject
	{
	public:
		virtual void GenerateGUID()
		{
			// todo
		}
		_NODISCARD_ GUID GetGUID() const { return Guid; }
		void AddDependency(const GUID& guid) { Dependencies.push_back(guid); }

	private:
		GUID Guid {};
		NameHandle ResourceName; // 资源的虚拟路径（如 "/Game/Textures/Texture1"）
		TArray<GUID> Dependencies; // 该资源需要的其他资源的guid，加载的时候需要一起加载
	};
}
