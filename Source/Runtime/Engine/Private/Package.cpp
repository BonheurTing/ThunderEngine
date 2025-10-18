#pragma optimize("", off) 
#include "Package.h"
#include "CRC.h"
#include "Guid.h"
#include "Mesh.h"
#include "ResourceModule.h"
#include "Texture.h"
#include "Concurrent/TaskScheduler.h"
#include "FileSystem/File.h"
#include "FileSystem/FileModule.h"
#include "FileSystem/FileSystem.h"

namespace Thunder
{
	Package::Package(const NameHandle& name) : GameObject(), 
		PackageName(name)
	{
		Header.MagicNumber = 0x50414745; // "PAGE" in ASCII
		Header.CheckSum = 0; // 初始校验和为0
		Header.Version = static_cast<uint32>(EPackageVersion::First); // 初始版本号
		TGuid::GenerateGUID(Header.Guid); // 生成一个新的GUID
	}

	uint32 Package::CalculateHeaderSize() const
	{
		const auto numGuids = static_cast<uint32>(Header.GuidList.size());
		const uint32 size = sizeof(Header.MagicNumber)
			+ sizeof(Header.CheckSum)
			+ sizeof(Header.Version)
			+ sizeof(uint32) * 4 // GUID
			+ sizeof(uint32) // num of Guid
			+ sizeof(uint32) * 4 * numGuids // GUID列表
			+ sizeof(uint32) * numGuids // 偏移列表
			+ sizeof(uint32) * numGuids // 大小列表
			+ sizeof(uint32) * numGuids; // 类型列表（如果需要的话）
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
		for(uint32 i = 0; i < numGuids; ++i)
		{
			const GameResource* res = Objects[i];
			Header.GuidList[i] = res->GetGUID();
		}
		// Prepare the objects header data.
		TArray<uint32> offsetList, sizeList, typeList;
		offsetList.resize(numGuids);
		sizeList.resize(numGuids);
		typeList.resize(numGuids);
		// Serialize Header
		MemoryWriter curArchive;
		Serialize(curArchive);
		const uint32 headerSize = static_cast<uint32>(curArchive.Size());
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
		for (uint32 i = 0, flag = headerSize; i < numGuids; i++, flag += 12)
		{
			memcpy(fileDataPtr + flag, &offsetList[i], 4);
			memcpy(fileDataPtr + flag + 4, &sizeList[i], 4);
			memcpy(fileDataPtr + flag + 8, &typeList[i], 4);
		}
		// Calculate and write checksum
		Header.CheckSum = FCrc::BinaryCrc32(fileDataPtr + 8, fileSize - 8);
		memcpy(fileDataPtr + 4, &Header.CheckSum, 4);

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
		const uint32 numGuids = static_cast<uint32>(Header.GuidList.size());
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

		// 验证魔数
		if (Header.MagicNumber != 0x50414745) // "PAGE"
		{
			TMemory::Destroy(fileData);
			return false;
		}
		
		// 验证校验和（跳过魔数和校验和本身）
		uint8* fileDataPtr = static_cast<uint8*>(fileData);
		const uint32 calculatedChecksum = FCrc::BinaryCrc32(fileDataPtr + 8, fileSize - 8);
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
				gameResource = new Material(); //todo
			}
			else
			{
				TMemory::Destroy(fileData);
				return false; // 不支持的资源类型
			}
 			gameResource->DeSerialize(objectArchive);

			Objects[i] = gameResource;
		}
		
		// 注册包本身到资源管理器
		ResourceModule::GetModule()->RegisterPackage(this);
		LOG("load package : %s complete, resource count: %llu", fullPath.c_str(), Objects.size());

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
		auto& loadedResByPath = GetModule()->LoadedResourcesByPath;
		auto& loadedRes = GetModule()->LoadedResources;
		if (!bForce && IsLoaded(softPath))
		{
			const TGuid guid = loadedResByPath[softPath];
			if (loadedRes.contains(guid))
			{
				if (const auto pak = static_cast<Package*>(loadedRes[guid]))
				{
					outResources = pak->GetPackageObjects();
					return true;
				}
				TAssertf(false, "ResourceModule::LoadSync: Loaded resource is not a Package, softPath: %s", softPath.c_str());
				return false;
			}
		}

		const auto newPackage = new Package(softPath);
		if (newPackage->Load())
		{
			outResources = newPackage->GetPackageObjects();
			LOG("get package resources count: %llu", outResources.size());
			return true;
		}
		
		TAssertf(false, "ResourceModule::LoadSync: fail to load a Package, softPath: %s", softPath.c_str());
		delete newPackage;
		return false;
	}

	GameResource* ResourceModule::LoadSync(const TGuid& guid, bool bForce)
	{
		if (!GetModule()->ResourcePathMap.contains(guid))
		{
			TAssertf(false, "ResourceModule::LoadSync: fail to load a resource by guid: %u-%u-%u-%u", guid.A, guid.B, guid.C, guid.D);
			return nullptr;
		}
		return LoadSync(GetModule()->ResourcePathMap[guid], bForce);
	}

	GameResource* ResourceModule::LoadSync(const NameHandle& resourceSoftPath, bool bForce)
	{
		auto& loadedResByPath = GetModule()->LoadedResourcesByPath;
		auto& loadedRes = GetModule()->LoadedResources;
		if (!bForce && IsLoaded(resourceSoftPath))
		{
			const TGuid guid = loadedResByPath[resourceSoftPath];
			if (loadedRes.contains(guid))
			{
				const auto res = static_cast<GameResource*>(loadedRes[guid]);
				TAssertf(res != nullptr, "ResourceModule::LoadSync: Loaded resource is not a GameResource, softPath: %s", resourceSoftPath.c_str());
				return res;
			}
		}
		const String pakFullPath = ConvertSoftPathToFullPath(resourceSoftPath.ToString());
		const String pakSoftPath = CovertFullPathToSoftPath(pakFullPath);
		
		const auto newPackage = new Package(pakSoftPath);
		if (newPackage->Load())
		{
			const TGuid guid = loadedResByPath[resourceSoftPath];
			const auto res = static_cast<GameResource*>(loadedRes[guid]);
			TAssertf(res != nullptr, "ResourceModule::LoadSync: Loaded resource is not a GameResource, softPath: %s", resourceSoftPath.c_str());
			return res;
		}

		TAssertf(false, "ResourceModule::LoadSync: fail to load a Package, softPath: %s", pakSoftPath.c_str());
		delete newPackage;
		return nullptr;
	}

	void ResourceModule::LoadAsync(const TGuid& guid)
	{
		if (!GetModule()->ResourcePathMap.contains(guid))
		{
			TAssertf(false, "ResourceModule::LoadAsync: fail to load a resource by guid: %u-%u-%u-%u", guid.A, guid.B, guid.C, guid.D);
		}
		return LoadAsync(GetModule()->ResourcePathMap[guid]);
	}

	void ResourceModule::LoadAsync(const NameHandle& path)
	{
		GAsyncWorkers->PushTask([path]()
		{
			TArray<GameResource*> outResources;
			LoadSync(path, outResources, false);
			LOG("load game resource : %s async complete, resource count: %llu", path.ToString().c_str(), outResources.size());
			for (const auto res : outResources)
			{
				res->OnResourceLoaded();
			}
		});
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

		const TArray<GameResource*> objects = package->GetPackageObjects();
		for (auto obj : objects)
		{
			TGuid objGuid = obj->GetGUID();
			LoadedResources.emplace(objGuid, obj);
			LoadedResourcesByPath.emplace(obj->GetResourceName(), objGuid);
		}
	}

	void ResourceModule::ForAllResources(const TFunction<void(const TGuid&, NameHandle)>& Function)
	{
		const auto& resMap = GetModule()->ResourcePathMap;
		for (auto& pair : resMap)
		{
			Function(pair.first, pair.second);
		}
	}
}