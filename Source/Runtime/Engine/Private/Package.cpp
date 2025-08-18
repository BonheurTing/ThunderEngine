#include "Package.h"

#include "CRC.h"
#include "ResourceModule.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "FileSystem/FileSystem.h"

namespace Thunder
{
	void Package::Serialize(MemoryWriter& archive)
	{
		archive << Header.MagicNumber;
		archive << Header.Version;
		archive << Header.Guid;
		archive << Header.NumGUIDs;
		archive << Header.GuidList;
		archive << Header.OffsetSizeList;
		archive << Header.CheckSum;
	}

	bool Package::Save(const String& fullPath)
	{
		TAssert(!(fullPath.empty() || fullPath == "/"));

		Header.NumGUIDs = 0;
		Header.GuidList.clear();
		TArray<void*> objectData;
		for(int i = 0, offset = 0; i < Objects.size(); ++i)
		{
			if(const auto res = static_cast<GameResource*>(Objects[i]))
			{
				res->GenerateGUID(); // 确保资源有GUID
				Header.GuidList.push_back(res->GetGUID());
				Header.NumGUIDs++;
				
				uint32 size = 0;
				MemoryWriter archive;
				res->Serialize(archive);
				objectData.push_back(archive.Data());
				size = archive.Size();

				Header.OffsetSizeList.push_back(std::make_pair(offset, size));
				offset += size;
			}
		}

		MemoryWriter archive;
		Serialize(archive);

		int fileSize = archive.Size() + Header.OffsetSizeList[Header.NumGUIDs-1].first + 
			Header.OffsetSizeList[Header.NumGUIDs-1].second;
		void* fileData = TMemory::Malloc<uint8>(fileSize);
		memcpy(fileData, archive.Data(), archive.Size());
		for(int i = 0; i < Header.NumGUIDs; ++i)
		{
			memcpy(fileData + archive.Size() + Header.OffsetSizeList[i].first, 
				objectData[i], Header.OffsetSizeList[i].second);
		}
		Header.CheckSum = FCrc::StrCrc32(static_cast<uint8*>(fileData) + 8);
		memcpy(fileData + 4, &Header.CheckSum, 4);

		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		const String tempPath = fullPath + ".tmp";
		TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(tempPath, true));
		file->Write(fileData, fileSize);

		return file->Rename(fullPath); // 包含close
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
