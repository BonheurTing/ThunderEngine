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

		// 验证魔数
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
			gameResource->SetGuid(Header.GuidList[i]);
			String resourceName = PackageName.ToString() + "." + resourceSuffixList[i];
			gameResource->SetResourceName(resourceName);
			Objects[i] = gameResource;
		}
		
		// 注册包本身到资源管理器
		ResourceModule::GetModule()->RegisterPackage(this);
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
		file->Close();
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
			TMemory::Destroy(fileData);
			return false;
		}
		headerArchive >> checkSum; // 读取校验和
		headerArchive >> version; // 读取版本号
		headerArchive >> outGuid; // 读取GUID

		String softPath = ResourceModule::CovertFullPathToSoftPath(fullPath);
		ResourceModule::AddResourcePathPair(outGuid, softPath);

		uint32 numGuids;
		headerArchive >> numGuids;
		TArray<TGuid> guidList;
		guidList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			TGuid guid;
			headerArchive >> guid;
			guidList[i] = guid;
		}
		TArray<String> resourceSuffixList;
		resourceSuffixList.resize(numGuids);
		for (uint32 i = 0; i < numGuids; ++i)
		{
			headerArchive >> resourceSuffixList[i];

			String resSoftPath = ResourceModule::CovertFullPathToSoftPath(fullPath, resourceSuffixList[i]);
			ResourceModule::AddResourcePathPair(guidList[i], resSoftPath);
		}

		TMemory::Destroy(fileData);
		return true;
	}

	bool ResourceModule::LoadSync(const TGuid& guid, TArray<GameResource*>& outResources, bool bForce)
	{
		if (!bForce)
		{
			auto loadedRes = TryGetLoadedResource(guid);
			if (loadedRes != nullptr)
			{
				const auto pak = static_cast<Package*>(loadedRes);
				TAssertf(pak != nullptr, "ResourceModule::LoadSync: Loaded resource is not a Package");
				outResources = pak->GetPackageObjects();
				return true;
			}
		}

		return ForceLoadBySoftPath(GetModule()->ResourcePathMap[guid]);
	}

	GameResource* ResourceModule::LoadSync(const TGuid& guid, bool bForce)
	{
		TAssertf(GetModule()->ResourcePathMap.contains(guid),
			"ResourceModule::LoadSync: fail to load a resource by guid: %u-%u-%u-%u", guid.A, guid.B, guid.C, guid.D);
		if (!bForce)
		{
			auto loadedRes = TryGetLoadedResource(guid);
			if (loadedRes != nullptr)
			{
				const auto res = static_cast<GameResource*>(loadedRes);
				TAssertf(res != nullptr, "ResourceModule::LoadSync: Loaded resource is not a GameResource");
				return res;
			}
		}
		if (ForceLoadBySoftPath(GetModule()->ResourcePathMap[guid]))
		{
			auto loadedRes = TryGetLoadedResource(guid);
			if (loadedRes != nullptr)
			{
				const auto res = static_cast<GameResource*>(loadedRes);
				TAssertf(res != nullptr, "ResourceModule::LoadSync: Loaded resource is not a GameResource");
				TArray<TGuid> dependencies;
				res->GetDependencies(dependencies);
				for (const auto& dependency : dependencies)
				{
					LoadSync(dependency, false);
				}
				return res;
			}
		}
		return nullptr;
	}

	bool ResourceModule::ForceLoadBySoftPath(const NameHandle& softPath)
	{
		const String pakFullPath = ConvertSoftPathToFullPath(softPath.ToString());
		const String pakSoftPath = CovertFullPathToSoftPath(pakFullPath);

		const auto newPackage = new Package(pakSoftPath);
		bool ret = newPackage->Load();
		delete newPackage;
		return ret;
	}

	void ResourceModule::LoadAsync(const TGuid& guid)
	{
		TAssertf(GetModule()->ResourcePathMap.contains(guid),
			"ResourceModule::LoadAsync: fail to load a resource by guid: %u-%u-%u-%u", guid.A, guid.B, guid.C, guid.D);

		GAsyncWorkers->PushTask([guid]()
		{
			TArray<GameResource*> outResources;
			LoadSync(guid, outResources, false);
			for (const auto res : outResources)
			{
				res->OnResourceLoaded();
			}
		});
	}
	
	GameObject* ResourceModule::TryGetLoadedResource(const TGuid& guid)
	{
		if (!IsLoaded(guid))
		{
			return nullptr;
		}
		return GetModule()->LoadedResources[guid];
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
#ifdef WITH_EDITOR
		LoadedResourcesByPath.emplace(package->GetPackageName(), pakGuid);
#endif

		const TArray<GameResource*> objects = package->GetPackageObjects();
		for (auto obj : objects)
		{
			TGuid objGuid = obj->GetGUID();
			LoadedResources.emplace(objGuid, obj);
#ifdef WITH_EDITOR
			LoadedResourcesByPath.emplace(obj->GetResourceName(), objGuid);
#endif
		}
	}

	void ResourceModule::ForAllResources(const TFunction<void(const TGuid&, NameHandle)>& function)
	{
		const auto& resMap = GetModule()->ResourcePathMap;
		for (auto& pair : resMap)
		{
			function(pair.first, pair.second);
		}
	}
}