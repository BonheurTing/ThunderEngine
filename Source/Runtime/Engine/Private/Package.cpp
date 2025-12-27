#pragma optimize("", off) 
#include "Package.h"
#include "CRC.h"
#include "Guid.h"
#include "Mesh.h"
#include "PackageModule.h"
#include "Texture.h"
#include "Concurrent/TaskScheduler.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "FileSystem/FileSystem.h"

namespace Thunder
{
	Package::Package(const NameHandle& name, const TGuid& inGuid) : GameObject(), 
		PackageName(name)
	{
		Header.MagicNumber = 0x50414745; // "PAGE" in ASCII
		Header.CheckSum = 0;
		Header.Version = static_cast<uint32>(EPackageVersion::First);
		if (inGuid.IsValid())
		{
			Header.Guid = inGuid;
		}
		else
		{
			TGuid::GenerateGUID(Header.Guid);
		}
	}

	void Package::Serialize(MemoryWriter& archive)
	{
		archive << Header.MagicNumber;
		archive << Header.CheckSum;
		archive << Header.Version;
		archive << Header.Guid;
		const auto numGuids = static_cast<uint32>(Header.GuidList.size());
		archive << numGuids;
		
		for (const TGuid& guid : Header.GuidList)
		{
			archive << guid;
		}
	}

	void Package::DeSerialize(MemoryReader& archive)
	{
		archive >> Header.MagicNumber;
		archive >> Header.CheckSum;
		archive >> Header.Version;
		archive >> Header.Guid;
		uint32 numGuids;
		archive >> numGuids;
		
		Header.GuidList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			TGuid guid;
			archive >> guid;
			Header.GuidList[i] = guid;
		}
	}

	bool Package::Save()
	{
		TAssert(!(PackageName.ToString().empty() || PackageName == "/"));
		String fullPath = PackageModule::ConvertSoftPathToFullPath(PackageName.ToString());

		// Prepare the package header.
		const auto numGuids = static_cast<uint32>(Objects.size());
		Header.GuidList.clear();
		Header.GuidList.resize(numGuids);
		for(uint32 i = 0; i < numGuids; ++i)
		{
			const GameResource* res = Objects[i];
			Header.GuidList[i] = res->GetGUID();
		}
		
		// Serialize Header
		MemoryWriter curArchive;
		uint32 headerSize = 0;
		curArchive << headerSize;
		Serialize(curArchive);
		for (auto obj : Objects)
		{
			String resourceSuffix = FileModule::GetFileExtension(obj->GetResourceName().ToString());
			curArchive << resourceSuffix;
		}
		headerSize = static_cast<uint32>(curArchive.Size());

		// Prepare the objects header data.
		TArray<uint32> offsetList, sizeList, typeList;
		offsetList.resize(numGuids);
		sizeList.resize(numGuids);
		typeList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			curArchive << offsetList[i];
			curArchive << sizeList[i];
			curArchive << typeList[i];
		}
		// Serialize Objects
		uint32 offset = static_cast<uint32>(curArchive.Size());
		for(uint32 i = 0; i < numGuids; ++i)
		{
			GameResource* res = Objects[i];
			res->Serialize(curArchive);
			offsetList[i] = offset;
			sizeList[i] = static_cast<uint32>(curArchive.Size()) - offset;
			typeList[i] = static_cast<uint32>(res->GetResourceType()); // 如果需要类型信息
			offset += sizeList[i];
		}
		// Copy objects header data
		const uint32 fileSize = offset;
		void* fileData = curArchive.Data();
		uint8* fileDataPtr = static_cast<uint8*>(fileData);
		memcpy(fileDataPtr, &headerSize, 4);
		for (uint32 i = 0, flag = headerSize; i < numGuids; i++, flag += 12)
		{
			memcpy(fileDataPtr + flag, &offsetList[i], 4);
			memcpy(fileDataPtr + flag + 4, &sizeList[i], 4);
			memcpy(fileDataPtr + flag + 8, &typeList[i], 4);
		}
		// Calculate and write checksum
		Header.CheckSum = FCrc::BinaryCrc32(fileDataPtr + 12, fileSize - 12);
		memcpy(fileDataPtr + 8, &Header.CheckSum, 4);

		// Write to .tmp file and rename it to the final path
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		const String tempPath = fullPath + ".tmp";
		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(tempPath, false));
		const size_t ret = file->Write(fileData, fileSize);
		return ret == fileSize && file->Rename(tempPath, fullPath); // 包含close
	}

	bool Package::Load()
	{
		TAssertf(!PackageName.IsEmpty(), "Package::Load: PackageName is empty, cannot load package.");
		const String fullPath = PackageModule::ConvertSoftPathToFullPath(PackageName.ToString(), "tasset");

		// read file
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		if (!fileSystem)
		{
			return false;
		}

		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(fullPath, false));
		const size_t fileSize = file->Size();
		if (fileSize == 0)
		{
			return false;
		}

		void* fileData = TMemory::Malloc<uint8>(fileSize);
		const size_t bytesRead = file->Read(fileData, fileSize);
		file->Close();

		if (bytesRead != fileSize)
		{
			TMemory::Destroy(fileData);
			return false;
		}

		BinaryData binaryData;
		binaryData.Data = fileData;
		binaryData.Size = fileSize;
		
		// DeSerialize
		MemoryReader headerArchive(&binaryData);
		uint32 headerSize = 0;
		headerArchive >> headerSize;
		DeSerialize(headerArchive);
		const uint32 numGuids = static_cast<uint32>(Header.GuidList.size());
		TArray<String> resourceSuffixList;
		resourceSuffixList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			headerArchive >> resourceSuffixList[i];
		}

		TArray<uint32> offsetList, sizeList;
		TArray<ETempGameResourceReflective> typeList;
		offsetList.resize(numGuids);
		sizeList.resize(numGuids);
		typeList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			headerArchive >> offsetList[i];
			headerArchive >> sizeList[i];
			uint32 type;
			headerArchive >> type;
			typeList[i] = static_cast<ETempGameResourceReflective>(type);
		}

		// check data
		if (Header.MagicNumber != 0x50414745) // "PAGE"
		{
			TMemory::Destroy(fileData);
			return false;
		}
		
		// 验证校验和（跳过魔数和校验和本身）
		uint8* fileDataPtr = static_cast<uint8*>(fileData);
		const uint32 calculatedChecksum = FCrc::BinaryCrc32(fileDataPtr + 12, fileSize - 12);
		if (Header.CheckSum != calculatedChecksum)
		{
			TMemory::Destroy(fileData);
			return false;
		}

		Objects.resize(numGuids);

		// 反序列化每个对象
		for (uint32 i = 0; i < numGuids; ++i)
		{
			// 创建BinaryData来包装对象数据
			const void* objectDataStart = fileDataPtr + offsetList[i];
			BinaryData objectBinaryData;
			objectBinaryData.Data = const_cast<void*>(objectDataStart);
			objectBinaryData.Size = sizeList[i];
			MemoryReader objectArchive(&objectBinaryData);
			
			GameResource* gameResource;
			if (typeList[i] == ETempGameResourceReflective::StaticMesh)
			{
				gameResource = new StaticMesh();
			}
			else if (typeList[i] == ETempGameResourceReflective::Texture2D)
			{
				gameResource = new Texture2D();
			}
			else if (typeList[i] == ETempGameResourceReflective::Material)
			{
				gameResource = new GameMaterial();
			}
			else
			{
				TMemory::Destroy(fileData);
				return false;
			}
			gameResource->DeSerialize(objectArchive);
			gameResource->SetOuter(this);
			gameResource->SetGuid(Header.GuidList[i]);
			String resourceName = PackageName.ToString() + "." + resourceSuffixList[i];
			gameResource->SetResourceName(resourceName);
			Objects[i] = gameResource;
		}
		
		// 注册包本身到资源管理器
		LOG("load package : %s complete, resource count: %llu", fullPath.c_str(), Objects.size());

		TMemory::Destroy(fileData);
		return true;
	}

	bool Package::TraverseGuidInPackage(const String& fullPath, TGuid& outGuid)
	{
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		if (!fileSystem)
		{
			return false;
		}

		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(fullPath, false));
		TAssert(file->Size() >= 4);
		uint32 headerSize;
		size_t bytesRead = file->Read(&headerSize, 4);
		TAssert(bytesRead == 4);

		// read header
		TAssert(file->Size() >= headerSize);
		const size_t needSize = headerSize - 4;
		void* fileData = TMemory::Malloc<uint8>(needSize);
		bytesRead = file->Read(fileData, needSize);
		TAssert(bytesRead == needSize);

		BinaryData binaryData;
		binaryData.Data = fileData;
		binaryData.Size = needSize;

		// 创建MemoryReader来反序列化头部
		MemoryReader headerArchive(&binaryData);
		uint32 magicNumber, checkSum, version;
		headerArchive >> magicNumber; // 读取魔数
		if (magicNumber != 0x50414745) // "PAGE"
		{
			file->Close();
			TMemory::Destroy(fileData);
			return false;
		}
		headerArchive >> checkSum; // 读取校验和
		headerArchive >> version; // 读取版本号
		headerArchive >> outGuid; // 读取GUID

		String softPath = PackageModule::CovertFullPathToSoftPath(fullPath);
		auto newEntry = PackageModule::AddPackageEntry(outGuid, softPath);
		PackageModule::AddResourceToPackage(outGuid, outGuid); // pak -> pak

		uint32 numGuids;
		headerArchive >> numGuids;
		TArray<TGuid> guidList;
		guidList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			TGuid guid;
			headerArchive >> guid;
			guidList[i] = guid;

			PackageModule::AddResourceToPackage(guid, outGuid); // res -> pak
		}
		TArray<String> resourceSuffixList;
		resourceSuffixList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			headerArchive >> resourceSuffixList[i];
		}

		TArray<uint32> offsetList;
		offsetList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			uint32 offset = headerSize + i * sizeof(uint32) * 3;
			size_t readBytes = file->PRead(&offsetList[i], sizeof(uint32), offset);
			if (readBytes != sizeof(uint32))
			{
				file->Close();
				TMemory::Destroy(fileData);
				return false;
			}
		}

		// 读取每个对象的依赖关系信息
		for (uint32 i = 0; i < numGuids; ++i)
		{
			// 使用 PRead 从 offsetList[i] 位置读取依赖数据，不改变文件指针
			// 首先读取 dependencyCount (4 bytes)
			uint32 dependencyCount = 0;
			size_t readBytes = file->PRead(&dependencyCount, sizeof(uint32), offsetList[i]);
			if (readBytes != sizeof(uint32))
			{
				file->Close();
				TMemory::Destroy(fileData);
				return false;
			}

			// 读取所有依赖的 GUID
			TArray<TGuid> dependencies;
			dependencies.resize(dependencyCount);
			if (dependencyCount > 0)
			{
				const size_t guidDataSize = sizeof(TGuid) * dependencyCount;
				readBytes = file->PRead(dependencies.data(), guidDataSize, offsetList[i] + sizeof(uint32));
				if (readBytes != guidDataSize)
				{
					file->Close();
					TMemory::Destroy(fileData);
					return false;
				}
			}

			newEntry->Dependencies.emplace(guidList[i], dependencies);
		}

		file->Close();
		TMemory::Destroy(fileData);
		return true;
	}

	bool PackageModule::SavePackage(Package* package)
	{
		if (!package)
		{
			return false; // 如果包为空，则不保存
		}
		return package->Save();
	}
}
