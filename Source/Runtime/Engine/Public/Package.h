#pragma once
#include "Container.h"
#include "GameObject.h"

namespace Thunder
{
	
	class Package : public GameObject
	{
	public:
		Package(const String& name);

		void AddResource(GameObject* obj)
		{
			TAssert(obj != nullptr);
			Objects.push_back(obj);
		}

		struct AssetHeader
		{
			uint32 MagicNumber; // 魔数
			uint32 CheckSum; // 全局CRC校验和
			uint32 Version; // 版本号
			TGuid Guid; // 包的唯一标识符
			uint32 NumGUIDs; // 包含的需要序列化的对象数量
			TArray<TGuid> GuidList; // 包含的对象的GUID列表
			TArray<uint32> OffsetList; // 每个对象的偏移
			TArray<uint32> SizeList; // 每个对象的大小
		};

		void Serialize(MemoryWriter& archive) override;
		bool Save(const String& fullPath);
		bool Load();

		_NODISCARD_ TGuid GetGUID() const { return Header.Guid; }

	private:
		AssetHeader Header; // 包头信息
		NameHandle PackageName; // 虚拟路径（如 "/Game/Meshes/Chair"）

		TArray<GameObject*> Objects; // 包含的 GameObject 列表, 包含有guid的GameResource和没有guid的Component等
	};
}


