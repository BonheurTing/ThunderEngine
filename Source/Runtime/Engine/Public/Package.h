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
		Package(const NameHandle& name, const TGuid& inGuid);

		void AddResource(GameResource* obj)
		{
			TAssert(obj != nullptr);
			Objects.push_back(obj);
		}

		struct AssetHeader
		{
			//uint32 HeaderSize; // To only read head but no objects
			uint32 MagicNumber;
			uint32 CheckSum;
			uint32 Version;
			TGuid Guid;
			TArray<TGuid> GuidList;
			// TArray<String> ResourceSuffixList; // Suffix is the resource name of contained objects, SoftPath = PackageName.ResourceSuffix

			// --- HeaderSize : The size of the above data
			//TArray<uint32> OffsetList; // Offset of objects in this package.
			//TArray<uint32> SizeList; // Size of objects in this package.
			//TArray<uint8> TypeList; // Type of objects in this package.

			// For incremental import.
			int64 SrcFileLastWriteTime { 0 };
			int64 SrcFileSize { 0 };
		};

		void Serialize(MemoryWriter& archive);
		void DeSerialize(MemoryReader& archive);
		bool Save();
		bool Load();
		static bool BuildPackageEntry(const String& fullPath, TGuid& outGuid);

		_NODISCARD_ TGuid GetGUID() const { return Header.Guid; }
		_NODISCARD_ const NameHandle& GetPackageName() const { return PackageName; }
		_NODISCARD_ TArray<GameResource*>& GetPackageObjects() { return Objects; }

		void SetSourceFileInfo(int64 lastWriteTime, int64 fileSize)
		{
			Header.SrcFileLastWriteTime = lastWriteTime;
			Header.SrcFileSize = fileSize;
		}
		_NODISCARD_ int64 GetSourceLastWriteTime() const { return Header.SrcFileLastWriteTime; }
		_NODISCARD_ int64 GetSourceFileSize() const { return Header.SrcFileSize; }

	private:
		AssetHeader Header;
		NameHandle PackageName; // SoftPath example : "/Game/Meshes/Chair"

		TArray<GameResource*> Objects;
	};
}


