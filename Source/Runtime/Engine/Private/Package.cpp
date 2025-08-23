#pragma optimize("", off) 
#include "Package.h"
#include "CRC.h"
#include "Guid.h"
#include "Mesh.h"
#include "ResourceModule.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "FileSystem/FileSystem.h"

namespace Thunder
{
	Package::Package(const NameHandle& name) : GameObject(), 
		PackageName(name)
	{
		Header.MagicNumber = 0x50414745; // "PAG" in ASCII
		Header.CheckSum = 0; // 初始校验和为0
		Header.Version = THUNDER_ENGINE_VERSION; // 初始版本号
		TGuid::GenerateGUID(Header.Guid); // 生成一个新的GUID
	}

	uint32 Package::CalculateHeaderSize() const
	{
		const auto numGuids = static_cast<uint32>(Header.GuidList.size());
		const uint32 size = sizeof(Header.MagicNumber)
			+ sizeof(Header.CheckSum)
			+ sizeof(Header.Version)
			+ sizeof(Header.Guid)
			+ sizeof(uint32) // num of Guid
			+ sizeof(uint32) * 4 * numGuids // GUID列表
			+ sizeof(uint32) * numGuids // 偏移列表
			+ sizeof(uint32) * numGuids; // 大小列表
		return size;
	}

	void Package::Serialize(MemoryWriter& archive)
	{
		archive << Header.MagicNumber;
		archive << Header.CheckSum;
		archive << Header.Version;
		archive << Header.Guid;
		const auto numGuids = static_cast<uint32>(Header.GuidList.size());
		archive << numGuids;
		
		// 手动序列化TArray
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
		
		// 手动反序列化TArray
		Header.GuidList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			TGuid guid;
			archive >> guid;
			Header.GuidList[i] = guid;
		}
	}

	bool Package::Save(const String& fullPath)
	{
		TAssert(!(fullPath.empty() || fullPath == "/"));
		TAssert(Header.GuidList.empty());

		// Prepare the package header.
		const auto numGuids = static_cast<uint32>(Objects.size());
		Header.GuidList.clear();
		Header.GuidList.resize(numGuids);
		const uint32 headerSize = CalculateHeaderSize();
		// Prepare the objects data.
		TArray<uint32> OffsetList, SizeList;
		OffsetList.resize(numGuids);
		SizeList.resize(numGuids);
		TArray<void*> objectData;
		objectData.resize(numGuids);
		uint32 offset = headerSize;
		for(uint32 i = 0; i < numGuids; ++i)
		{
			GameResource* res = Objects[i];
			Header.GuidList[i] = res->GetGUID();

			MemoryWriter archive;
			res->Serialize(archive);
			objectData[i] = archive.Data();
			const auto objectSize = static_cast<uint32>(archive.Size()); 

			OffsetList[i] = offset;
			SizeList[i] = objectSize;
			offset += 4 + objectSize; // crc + data size
		}

		// Serialize Header
		MemoryWriter headerArchive;
		Serialize(headerArchive);
		for (const uint32& objOffset : OffsetList)
		{
			headerArchive << objOffset;
		}
		
		for (const uint32& size : SizeList)
		{
			headerArchive << size;
		}
		TAssert(headerSize == headerArchive.Size());

		const size_t fileSize = offset; // 4 for the last object CRC
		void* fileData = TMemory::Malloc<uint8>(fileSize);
		memcpy(fileData, headerArchive.Data(), headerSize);

		// Serialize Objects
		for(uint32 i = 0; i < numGuids; ++i)
		{
			uint32 objectCheckSum = FCrc::StrCrc32(static_cast<uint8*>(objectData[i]));
			memcpy(static_cast<uint8*>(fileData) + OffsetList[i], &objectCheckSum, 4);
			memcpy(static_cast<uint8*>(fileData) + OffsetList[i] + 4, objectData[i], SizeList[i]);
		}
		Header.CheckSum = FCrc::StrCrc32(static_cast<uint8*>(fileData) + 8);
		memcpy(static_cast<uint8*>(fileData) + 4, &Header.CheckSum, 4);

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
		const String fullPath = ResourceModule::ConvertSoftPathToFullPath(PackageName.ToString(), "tasset");

		// 打开文件
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		if (!fileSystem)
		{
			return false;
		}
		
		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(fullPath, false));
		// 获取文件大小并读取全部内容
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

		// 创建BinaryData来包装文件数据
		BinaryData binaryData;
		binaryData.Data = fileData;
		binaryData.Size = fileSize;
		
		// 创建MemoryReader来反序列化头部
		MemoryReader headerArchive(&binaryData);
		DeSerialize(headerArchive);
		const auto numGuids = static_cast<uint32>(Header.GuidList.size());
		TArray<uint32> OffsetList, SizeList;
		OffsetList.resize(numGuids);
		SizeList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			uint32 offset;
			headerArchive >> offset;
			OffsetList[i] = offset;
		}
		
		for (uint32 i = 0; i < numGuids; ++i)
		{
			uint32 size;
			headerArchive >> size;
			SizeList[i] = size;
		}

		// 验证魔数
		if (Header.MagicNumber != 0x50414745) // "PAGE"
		{
			TMemory::Destroy(fileData);
			return false;
		}
		
		// 验证校验和（跳过魔数和校验和本身）
		const uint32 calculatedChecksum = FCrc::StrCrc32(static_cast<uint8*>(fileData) + 8);
		if (Header.CheckSum != calculatedChecksum)
		{
			TMemory::Destroy(fileData);
			return false;
		}

		// 清空之前的对象
		Objects.resize(numGuids);

		// 反序列化每个对象
		for (uint32 i = 0; i < numGuids; ++i)
		{
			const uint32 objectOffset = OffsetList[i];
			const uint32 objectSize = SizeList[i];
			
			// 验证对象的CRC校验和
			uint32 storedChecksum;
			memcpy(&storedChecksum, static_cast<const uint8*>(fileData) + objectOffset, 4);
			const void* objectDataStart = static_cast<const uint8*>(fileData) + objectOffset + 4;
			const uint32 calculatedObjectChecksum = FCrc::StrCrc32(static_cast<const uint8*>(objectDataStart), 0, objectSize);
			
			if (storedChecksum != calculatedObjectChecksum)
			{
				TMemory::Destroy(fileData);
				return false;
			}

			// 创建BinaryData来包装对象数据
			BinaryData objectBinaryData;
			objectBinaryData.Data = const_cast<void*>(objectDataStart);
			objectBinaryData.Size = objectSize;

			MemoryReader objectArchive(&objectBinaryData);
			
			// 根据GUID类型创建对应的对象（这里假设都是StaticMesh，实际情况可能需要类型识别）
			// TODO: 需要添加类型识别机制来确定创建哪种类型的对象
			auto* gameResource = new StaticMesh();
			gameResource->DeSerialize(objectArchive);
			
			Objects[i] = gameResource;
		}
		
		// 注册包本身到资源管理器
		ResourceModule::GetModule()->RegisterPackage(this);

		TMemory::Destroy(fileData);
		return true;
	}

	bool Package::LoadOnlyGuid(const String& fullPath, TGuid& outGuid)
	{
		IFileSystem* fileSystem = FileModule::GetFileSystem("Content");
		if (!fileSystem)
		{
			return false;
		}
		
		const TRefCountPtr<NativeFile> file = static_cast<NativeFile*>(fileSystem->Open(fullPath, false));
		// 获取文件大小并读取全部内容
		constexpr size_t neededSize = 28;
		if (file->Size() <= neededSize)
		{
			return false;
		}

		void* fileData = TMemory::Malloc<uint8>(neededSize);
		const size_t bytesRead = file->Read(fileData, neededSize);
		file->Close();

		if (bytesRead != neededSize)
		{
			TMemory::Destroy(fileData);
			return false;
		}

		BinaryData binaryData;
		binaryData.Data = fileData;
		binaryData.Size = neededSize;
		
		// 创建MemoryReader来反序列化头部
		MemoryReader headerArchive(&binaryData);
		uint32 magicNumber, checkSum, version;
		headerArchive >> magicNumber; // 读取魔数
		if (magicNumber != 0x50414745) // "PAGE"
		{
			TMemory::Destroy(fileData);
			return false;
		}
		headerArchive >> checkSum; // 读取校验和
		headerArchive >> version; // 读取版本号
		headerArchive >> outGuid; // 读取GUID

		TMemory::Destroy(fileData);
		return true;
	}

	bool ResourceModule::LoadSync(const NameHandle& softPath, TArray<GameResource*>& outResources, bool bForce)
	{
		if (!bForce && IsLoaded(softPath))
		{
			const TGuid guid = LoadedResourcesByPath[softPath];
			if (LoadedResources.contains(guid))
			{
				if (const auto pak = static_cast<Package*>(LoadedResources[guid]))
				{
					pak->GetPackageObjects(outResources);
					return true;
				}
				TAssertf(false, "ResourceModule::LoadSync: Loaded resource is not a Package, softPath: %s", softPath.c_str());
				return false;
			}
		}

		const auto newPackage = new Package(softPath);
		if (newPackage->Load())
		{
			newPackage->GetPackageObjects(outResources);
			return true;
		}

		return false;
	}

	GameResource* ResourceModule::LoadSync(const NameHandle& resourceSoftPath, bool bForce)
	{
		if (!bForce && IsLoaded(resourceSoftPath))
		{
			const TGuid guid = LoadedResourcesByPath[resourceSoftPath];
			if (LoadedResources.contains(guid))
			{
				const auto res = static_cast<GameResource*>(LoadedResources[guid]);
				TAssertf(res != nullptr, "ResourceModule::LoadSync: Loaded resource is not a GameResource, softPath: %s", resourceSoftPath.c_str());
				return res;
			}
		}
		const String pakFullPath = ConvertSoftPathToFullPath(resourceSoftPath.ToString());
		const String pakSoftPath = CovertFullPathToSoftPath(pakFullPath);
		
		const auto newPackage = new Package(pakSoftPath);
		if (newPackage->Load())
		{
			const TGuid guid = LoadedResourcesByPath[resourceSoftPath];
			const auto res = static_cast<GameResource*>(LoadedResources[guid]);
			TAssertf(res != nullptr, "ResourceModule::LoadSync: Loaded resource is not a GameResource, softPath: %s", resourceSoftPath.c_str());
			return res;
		}
		return nullptr;
	}

	void ResourceModule::LoadAsync(const NameHandle& path)
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

	void ResourceModule::RegisterPackage(Package* package)
	{
		TGuid pakGuid = package->GetGUID();
		LoadedResources.emplace(pakGuid, package);
		LoadedResourcesByPath.emplace(package->GetPackageName(), pakGuid);

		TArray<GameResource*> objects;
		package->GetPackageObjects(objects);
		for (auto obj : objects)
		{
			TGuid objGuid = obj->GetGUID();
			LoadedResources.emplace(objGuid, obj);
			LoadedResourcesByPath.emplace(obj->GetResourceName(), objGuid);
		}
	}
}
#pragma optimize("", on) 