#pragma optimize("", off) 
#include "Package.h"
#include "CRC.h"
#include "Guid.h"
#include "ResourceModule.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "FileSystem/FileSystem.h"

namespace Thunder
{
	Package::Package(const String& name) : GameObject(), 
		PackageName(name)
	{
		Header.MagicNumber = 0x50414745; // "PAG" in ASCII
		Header.CheckSum = 0; // 初始校验和为0
		Header.Version = THUNDER_ENGINE_VERSION; // 初始版本号
		TGuid::GenerateGUID(Header.Guid); // 生成一个新的GUID
		Header.NumGUIDs = 0;
	}

	void Package::Serialize(MemoryWriter& archive)
	{
		archive << Header.MagicNumber;
		archive << Header.CheckSum;
		archive << Header.Version;
		archive << Header.Guid;
		archive << Header.NumGUIDs;
		archive << Header.GuidList;
		archive << Header.OffsetList;
		archive << Header.SizeList;
	}

	bool Package::Save(const String& fullPath)
	{
		TAssert(!(fullPath.empty() || fullPath == "/"));
		TAssert(Header.NumGUIDs == 0 && Header.GuidList.empty() && Header.OffsetList.empty() && Header.SizeList.empty());

		TArray<void*> objectData;
		for(uint32 i = 0, offset = 0; i < static_cast<uint32>(Objects.size()); ++i)
		{
			if(const auto res = static_cast<GameResource*>(Objects[i]))
			{
				TGuid guid = res->GetGUID();
				Header.GuidList.push_back(guid);
				Header.NumGUIDs++;
				
				uint32 size = 0;
				MemoryWriter archive;
				res->Serialize(archive);
				objectData.push_back(archive.Data());
				size = static_cast<uint32>(archive.Size());

				Header.OffsetList.push_back(offset);
				Header.SizeList.push_back(size);
				offset += size;
			}
		}

		MemoryWriter archive;
		Serialize(archive);

		const size_t fileSize = archive.Size() + Header.OffsetList[Header.NumGUIDs-1] + 
			Header.SizeList[Header.NumGUIDs-1];
		void* fileData = TMemory::Malloc<uint8>(fileSize);
		memcpy(fileData, archive.Data(), archive.Size());
		for(uint32 i = 0; i < Header.NumGUIDs; ++i)
		{
			memcpy(static_cast<uint8*>(fileData) + archive.Size() + Header.OffsetList[i], 
				objectData[i], Header.SizeList[i]);
		}
		Header.CheckSum = FCrc::StrCrc32(static_cast<uint8*>(fileData) + 8);
		memcpy(static_cast<uint8*>(fileData) + 4, &Header.CheckSum, 4);

		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		const String tempPath = fullPath + ".tmp";
		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(tempPath, true));
		const size_t ret = file->Write(fileData, fileSize);
		return ret == fileSize && file->Rename(tempPath, fullPath); // 包含close
	}

	bool Package::Load()
	{
		return false; // 这里可以实现加载逻辑
	}

	void ResourceModule::LoadSync(const String& path)
	{
	}

	void ResourceModule::LoadAsync(const String& path)
	{
	}

	
	bool ResourceModule::SavePackage(Package* package, const String& fullPath)
	{
		if (!package)
		{
			return false; // 如果包为空，则不保存
		}
		return package->Save(fullPath);
	}
	
}
#pragma optimize("", on) 