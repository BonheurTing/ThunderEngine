#pragma once
#include "Container.h"
#include "GameObject.h"

namespace Thunder
{
	enum class EPackageVersion : uint32
	{
		First = 0
	};
	
	class Package : public GameObject
	{
	public:
		Package(const NameHandle& name);

		void AddResource(GameResource* obj)
		{
			TAssert(obj != nullptr);
			Objects.push_back(obj);
		}

		struct AssetHeader
		{
			//uint32 HeaderSize; // 0 - ResourceSuffixList
			uint32 MagicNumber; // 魔数
			uint32 CheckSum; // 全局CRC校验和
			uint32 Version; // 版本号
			TGuid Guid; // 包的唯一标识符
			//uint32 NumGUIDs; // 包含的需要序列化的对象数量
			TArray<TGuid> GuidList; // 包含的对象的GUID列表
			//TArray<String> ResourceSuffixList; // 包含的对象的资源名，虚拟路径中ResourceName = PackageName.ResourceSuffix

			// --- HeaderSize : The size of the above data
			//TArray<uint32> OffsetList; // 每个对象在胞体的绝对偏移
			//TArray<uint32> SizeList; // 每个对象的大小
			//TArray<uint8> TypeList; // 每个对象的类型
		};

		void Serialize(MemoryWriter& archive);
		void DeSerialize(MemoryReader& archive);
		bool Save(const String& fullPath);
		bool Load();
		static bool TraverseGuidInPackage(const String& fullPath, TGuid& outGuid);

		_NODISCARD_ TGuid GetGUID() const { return Header.Guid; }
		_NODISCARD_ const NameHandle& GetPackageName() const { return PackageName; }
		_NODISCARD_ TArray<GameResource*>& GetPackageObjects() { return Objects; }

	private:
		AssetHeader Header; // 包头信息
		NameHandle PackageName; // 虚拟路径（如 "/Game/Meshes/Chair"）

		TArray<GameResource*> Objects; // 包含的 GameObject 列表, 包含有guid的GameResource和没有guid的Component等 //todo 强引用
	};
}


