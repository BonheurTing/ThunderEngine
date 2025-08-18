#pragma once
#include "Container.h"
#include "GameObject.h"
#include "NameHandle.h"

namespace Thunder
{
	
	class Package : public GameObject
	{
	public:
		Package(const String& name) : PackageName(name)
		{
			Header.MagicNumber = 0x50414745; // "PAG" in ASCII
			Header.CheckSum = 0; // 初始校验和为0
			Header.Version = THUNDER_ENGINE_VERSION; // 初始版本号
			Header.Guid = GUID::Generate(); // 生成一个新的GUID
			Header.NumGUIDs = 0;
		}

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
			GUID Guid; // 包的唯一标识符
			uint32 NumGUIDs; // 包含的需要序列化的对象数量
			TArray<GUID> GuidList; // 包含的对象的GUID列表
			TArray<std::pair<uint32, uint32>> OffsetSizeList; // 每个对象的偏移和大小
		};

		void Serialize(MemoryWriter& archive) override;
		bool Save(const String& fullPath);
		bool Load();

		_NODISCARD_ GUID GetGUID() const { return Header.Guid; }

	private:
		AssetHeader Header; // 包头信息
		NameHandle PackageName; // 虚拟路径（如 "/Game/Meshes/Chair"）

		TArray<GameObject*> Objects; // 包含的 GameObject 列表, 包含有guid的GameResource和没有guid的Component等
	};
}


