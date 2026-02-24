#pragma optimize("", off)
#include "PackageModule.h"
#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"		// 定义 stb_image 实现
#include <assimp/Importer.hpp>      // 导入接口
#include <assimp/scene.h>           // 场景数据（模型、材质、动画）
#include <assimp/postprocess.h>     // 后处理选项（如三角化、优化）

#include "GameModule.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

#include "Mesh.h"
#include "Package.h"
#include "Texture.h"
#include "Material.h"
#include "Container/ReflectiveContainer.h"
#include "FileSystem/FileModule.h"
#include "Vector.h"
#include "Concurrent/TaskScheduler.h"

#include <filesystem>

namespace Thunder
{
	IMPLEMENT_MODULE(Package, PackageModule)

	static TReflectiveContainerRef GenerateVertexBuffer(const aiMesh* mesh)
	{
		auto vertexBuffer = MakeRefCount<ReflectiveContainer>();
		vertexBuffer->SetDataNum(mesh->mNumVertices);
		vertexBuffer->AddComponent<TVector3f>("Position");
		if (mesh->mNormals != nullptr)
		{
			vertexBuffer->AddComponent<TVector3f>("Normal");
		}
		if (mesh->mTangents != nullptr)
		{
			vertexBuffer->AddComponent<TVector3f>("Tangent");
		}
		if (mesh->mBitangents != nullptr)
		{
			vertexBuffer->AddComponent<TVector3f>("Binormal");
		}
		for (int j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; j++)
		{
			if (mesh->mTextureCoords[j] != nullptr)
			{
				TAssert(mesh->mNumUVComponents[j] == 2);
				vertexBuffer->AddComponent<TVector2f>("UV"+std::to_string(j));
			}
			else
			{
				break;
			}
		}
		for (int j = 0; j < AI_MAX_NUMBER_OF_COLOR_SETS; j++)
		{
			if (mesh->mColors[j] != nullptr)
			{
				vertexBuffer->AddComponent<TVector4f>("Color"+std::to_string(j));
			}
			else
			{
				break;
			}
		}
		vertexBuffer->Initialize();
		const size_t compStride = vertexBuffer->GetStride();
		for (size_t v = 0, offset = vertexBuffer->GetComponentOffset("Position"); v < mesh->mNumVertices; v++, offset += compStride)
		{
			aiVector3D vertex = mesh->mVertices[v];
			vertexBuffer->CopyData(&vertex, offset, sizeof(TVector3f));
		}
		if (mesh->mNormals != nullptr)
		{
			for (size_t v = 0, offset = vertexBuffer->GetComponentOffset("Normal"); v < mesh->mNumVertices; v++, offset += compStride)
			{
				aiVector3D normal = mesh->mNormals[v];
				vertexBuffer->CopyData(&normal, offset, sizeof(TVector3f));
			}
		}
		if (mesh->mTangents != nullptr)
		{
			for (size_t v = 0, offset = vertexBuffer->GetComponentOffset("Tangent"); v < mesh->mNumVertices; v++, offset += compStride)
			{
				aiVector3D tangent = mesh->mTangents[v];
				vertexBuffer->CopyData(&tangent, offset, sizeof(TVector3f));
			}
		}
		if (mesh->mBitangents != nullptr)
		{
			for (size_t v = 0, offset = vertexBuffer->GetComponentOffset("Binormal"); v < mesh->mNumVertices; v++, offset += compStride)
			{
				aiVector3D bitangent = mesh->mBitangents[v];
				vertexBuffer->CopyData(&bitangent, offset, sizeof(TVector3f));
			}
		}
		for (int j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; j++)
		{
			if (mesh->mTextureCoords[j] != nullptr)
			{
				for (size_t v = 0, offset = vertexBuffer->GetComponentOffset("UV"+std::to_string(j)); v < mesh->mNumVertices; v++, offset += compStride)
				{
					aiVector3D uv = mesh->mTextureCoords[j][v];
					vertexBuffer->CopyData(&uv, offset, sizeof(TVector2f));
				}
			}
			else
			{
				break;
			}
		}
		for (int j = 0; j < AI_MAX_NUMBER_OF_COLOR_SETS; j++)
		{
			if (mesh->mColors[j] != nullptr)
			{
				for (size_t v = 0, offset = vertexBuffer->GetComponentOffset("Color"+std::to_string(j)); v < mesh->mNumVertices; v++, offset += compStride)
				{
					aiColor4D color = mesh->mColors[j][v];
					vertexBuffer->CopyData(&color, offset, sizeof(TVector4f));
				}
			}
			else
			{
				break;
			}
		}
		return vertexBuffer;
	}

	static TReflectiveContainerRef GenerateIndicesBuffer(const aiMesh* mesh)
	{
		TReflectiveContainerRef indicesBuffer = MakeRefCount<ReflectiveContainer>();
		indicesBuffer->AddComponent<int>("Index");
		indicesBuffer->SetDataNum(static_cast<size_t>(mesh->mNumFaces) * 3); // 每个三角形有3个索引
		indicesBuffer->Initialize();
		// 提取索引数据（三角形）
		for (size_t i = 0, offset = 0; i < mesh->mNumFaces; i++)
		{
			constexpr size_t size = sizeof(int);
			const aiFace face = mesh->mFaces[i];
			for (unsigned int idx = 0; idx < face.mNumIndices; idx++, offset += size)
			{
				unsigned int index = face.mIndices[idx];
				indicesBuffer->CopyData(&index, offset, size);
			}
		}

		return (indicesBuffer);
	}

	void PackageModule::ShutDown()
	{
		//todo: wait all request task down

		GameModule::UnregisterTickable(this);
		// print all resource dependency guid
		LOG("--------- Loaded All Resources Begin ---------");
		for (const auto& pakEntry : PackageMap | std::views::values)
		{
			if (!pakEntry->IsLoaded())
			{
				continue;
			}
			LOG("PackageName: %s, GUID: %s", pakEntry->Package->GetPackageName().c_str(), pakEntry->Guid.ToString().c_str());
			for (auto res : pakEntry->Package->GetPackageObjects())
			{
				switch (res->GetResourceType())
				{
				case ETempGameResourceReflective::StaticMesh:
					LOG("Name: %s, Type: %s, GUID: %s", res->GetResourceName().c_str(), "StaticMesh", res->GetGUID().ToString().c_str());
					break;
				case ETempGameResourceReflective::Texture2D:
					LOG("Name: %s, Type: %s, GUID: %s", res->GetResourceName().c_str(), "Texture2D", res->GetGUID().ToString().c_str());
					break;
				case ETempGameResourceReflective::Material:
					LOG("Name: %s, Type: %s, GUID: %s", res->GetResourceName().c_str(), "Material", res->GetGUID().ToString().c_str());
					break;
				default:
					TAssertf(false, "Unknown resource type");
				}
				LOG("--- Dependencies GUID:");
				for (const auto& guid : res->GetDependencies())
				{
					LOG("--- %s", guid.ToString().c_str());
				}
			}
		}
		LOG("--------- Loaded All Resources End ---------\n");
	}

	void PackageModule::Tick()
	{
		if (!AcquireRequests.empty())
		{
			uint32 count = 0;
			while(!AcquireRequests.empty() && count++ < FrameMaxDelivering)
			{
				PackageEntry* entry = AcquireRequests.front();
				AcquireRequests.pop_front();

				entry->SetAcquiring(false);
				entry->Status |= static_cast<uint32>(EPackageStatus::Loading);

				GAsyncWorkers->PushTask([entry]{
					if (LoadSync(entry->Guid, false))
					{
						GGameScheduler->PushTask([entry]{
							// Mark as load completed
							entry->IsLoadCompletedAndWaitingForDependencies = true;

							// Check if we can enter CompletionList (all dependencies ready)
							if (entry->PendingDependencyCount == 0)
							{
								GetModule()->CompletionList.push_back(entry);
							}
						});
					}
					else
					{
						LOG("Failed to load package with guid: %s", entry->Guid.ToString().c_str());
					}
				});
			}
		}

		if (!CompletionList.empty())
		{
			uint32 count = 0;
			while (!CompletionList.empty() && count++ < FrameMaxDelivering)
			{
				PackageEntry* entry = CompletionList.front();
				CompletionList.pop_front();

				if (!entry->Package.IsValid()) [[unlikely]]
				{
					TAssertf(false, "Package is invalid for guid: %s", entry->Guid.ToString().c_str());
					continue;
				}

				// Call resource callbacks.
				for (auto res : entry->Package->GetPackageObjects())
				{
					res->OnResourceLoaded();
				}

				// Set loaded after callback is called.
				entry->SetLoaded(true);

				// Call user callbacks.
				for (auto& callback : entry->Callbacks)
				{
					callback();
				}
				entry->Callbacks.clear();
			}
		}
	}

	bool PackageModule::LoadSync(const TGuid& inGuid, bool bForce)
	{
		TGuid pakGuid = GetModule()->ResourceToPackage[inGuid];
		if (!bForce)
		{
			if (IsLoaded(inGuid))
			{
				return true;
			}
		}

		return GetModule()->PackageMap[pakGuid]->Package->Load();
	}

	void PackageModule::LoadAsync(const TGuid& inGuid, TFunction<void()>&& inFunction)
	{
		TGuid pakGuid = GetModule()->ResourceToPackage[inGuid];

		TAssert(GetModule()->PackageMap.contains(pakGuid));
		PackageEntry* entry = GetModule()->PackageMap[pakGuid];

		// If already loading or loaded.
		if (entry->Package)
		{
			if (entry->IsLoaded())
			{
				if (inFunction)
				{
					inFunction();
				}
				return;
			}

			// Add callback only.
			if (inFunction)
			{
				entry->Callbacks.push_back(std::move(inFunction));
			}

			// To make sure we're acquiring this package.
			GetModule()->PackageAcquire(entry);
			return;
		}

		auto pakSoftPath = GetModule()->PackageMap[pakGuid]->SoftPath;
		Package* newPak = new Package(pakSoftPath, pakGuid);
		entry->Package = TWeakObjectPtr(newPak);

		if (inFunction)
		{
			entry->Callbacks.push_back(std::move(inFunction));
		}

		// Get dependencies.
		TSet<TGuid> resourceDependencies = GetModule()->GetPackageDependencies(pakGuid);
		entry->PendingDependencyCount = static_cast<uint32>(resourceDependencies.size());

		// Load dependencies with callback to decrement counter
		for (const auto& depPakGuid : resourceDependencies)
		{
			LoadAsync(depPakGuid, [entry]() {
				entry->PendingDependencyCount--;
				// Check if we can enter CompletionList (load completed && all dependencies ready)
				if (entry->PendingDependencyCount == 0 && entry->IsLoadCompletedAndWaitingForDependencies)
				{
					entry->IsLoadCompletedAndWaitingForDependencies = false;
					GetModule()->CompletionList.push_back(entry);
				}
			});
		}

		// Start loading this package immediately (parallel with dependencies)
		GetModule()->PackageAcquire(entry);
	}

	PackageEntry* PackageModule::AddPackageEntry(const TGuid& guid, const NameHandle& path)
	{
		PackageEntry* newEntry = new (TMemory::Malloc<PackageEntry>()) PackageEntry(guid, path);
		GetModule()->PackageMap.emplace(guid, newEntry);
		return newEntry;
	}

	void PackageModule::ForAllResources(const TFunction<void(const TGuid&, NameHandle)>& function)
	{
		const auto& resMap = GetModule()->PackageMap;
		for (auto& pair : resMap)
		{
			function(pair.first, pair.second->SoftPath);
		}
	}

	bool PackageModule::IsLoaded(const TGuid& inGuid)
	{
		TGuid pakGuid = GetModule()->ResourceToPackage[inGuid];
		PackageEntry* pakEntry = GetModule()->PackageMap[pakGuid];
		return pakEntry->IsLoaded();
	}

#if WITH_EDITOR
	GameResource* PackageModule::GetResource(NameHandle softPath)
	{
		if (GetModule()->SoftPathToGuidMap.contains(softPath))
		{
			return TryGetLoadedResource(GetModule()->SoftPathToGuidMap[softPath]);
		}
		return nullptr;
	}

	TGuid PackageModule::GetGuidBySoftPath(NameHandle softPath)
	{
		auto const& softPathToGuidMap = GetModule()->SoftPathToGuidMap;
		auto it = softPathToGuidMap.find(softPath);
		if (it == softPathToGuidMap.end())
		{
			return TGuid();
		}
		return it->second;
	}
#endif

	GameResource* PackageModule::TryGetLoadedResource(const TGuid& inGuid)
	{
		if (!inGuid.IsValid())
		{
			return nullptr;
		}
		TGuid pakGuid = GetModule()->ResourceToPackage[inGuid];
		TAssert(pakGuid != inGuid);
		PackageEntry* pakEntry = GetModule()->PackageMap[pakGuid];
		if (!pakEntry->IsLoaded())
		{
			return nullptr;
		}
		for (auto res : pakEntry->Package->GetPackageObjects())
		{
			if (res->GetGUID() == inGuid)
			{
				return res;
			}
		}
		TAssert(false);
		return nullptr;
	}

	void PackageModule::PackageAcquire(PackageEntry* entry)
	{
		if (entry->IsAcquiring())
		{
			return;
		}
		// Do Acquire
		AcquireRequests.emplace_back(entry);
		entry->SetAcquiring(true);
	}

	/**
	 * 1. 方案一
	 * if (PackageModule::DependencyCache.contais(guid))
	 * {
	 *	   return DependencyCache[guid];
	 * }
	 * 2. 方案二
	 * 读硬盘上文件头获取依赖
	 **/
	TSet<TGuid> PackageModule::GetPackageDependencies(const TGuid& pakGuid)
	{
		return PackageMap[pakGuid]->GetResourceDependencies();
	}


	void PackageModule::InitResourcePathMap()
	{
		const auto contentDict = FileModule::GetResourceContentRoot();
		TArray<String> fileNames;
		FileModule::TraverseFileFromFolder(contentDict, fileNames);
		for (const auto& fileName : fileNames)
		{
			if (FileModule::GetFileExtension(fileName) == "tasset")
			{
				// only load guid
				TGuid guid;
				if (!Package::BuildPackageEntry(fileName, guid))
				{
					LOG("Failed to load package guid from %s", fileName.c_str());
				}
			}
		}
		GameModule::RegisterTickable(GetModule());
		// print all resource guid
		LOG("--------- Scan All Packages %d Begin ---------", static_cast<int32>(GetModule()->PackageMap.size()));
		for (const auto& [fst, snd] : GetModule()->PackageMap)
		{
			LOG("Name: %s, GUID: %s", snd->SoftPath.ToString().c_str(), fst.ToString().c_str());
		}
		LOG("--------- Scan All Packages End ---------\n");
	}

	String PackageModule::CovertFullPathToSoftPath(const String& fullPath, const String& resourceName)
	{
		// 查找"Content"字符串的位置
		size_t contentPos = fullPath.find("Content");
		if (contentPos == std::string::npos)
		{
			return "";  // 如果没有找到"Content"，返回空字符串
		}
		
		// 提取"Content"后的部分路径
		String relativePath = fullPath.substr(contentPos + 7); // "Content" = 7个字符
		
		// 将反斜杠替换为正斜杠（统一路径分隔符）
		for (auto& ch : relativePath)
		{
			if (ch == '\\')
			{
				ch = '/';
			}
		}
		
		// 确保路径以'/'开始，如果不是则添加
		if (!relativePath.empty() && relativePath[0] != '/')
		{
			relativePath = "/" + relativePath;
		}
		
		// 移除文件扩展名
		size_t dotPos = relativePath.find_last_of('.');
		size_t slashPos = relativePath.find_last_of('/');
		if (dotPos != std::string::npos && (slashPos == std::string::npos || dotPos > slashPos))
		{
			relativePath = relativePath.substr(0, dotPos);
		}
		
		// 构建软路径
		String softPath = "/Game" + relativePath;

		// 如果resourceName不为空，添加资源名称后缀
		if (!resourceName.empty())
		{
			softPath += "." + resourceName;
		}
		
		return softPath;
	}

	String PackageModule::ConvertSoftPathToFullPath(const String& softPath, const String& extension)
	{
		String workingPath = softPath;
		
		// 移除资源名称后缀（如果存在点号）
		size_t lastDotPos = workingPath.find_last_of('.');
		size_t lastSlashPos = workingPath.find_last_of('/');
		if (lastDotPos != std::string::npos && (lastSlashPos == std::string::npos || lastDotPos > lastSlashPos))
		{
			workingPath = workingPath.substr(0, lastDotPos);
		}
		
		// 检查是否以"/Game"开头
		if (workingPath.substr(0, 5) != "/Game")
		{
			return "";  // 不是有效的软路径格式
		}
		
		// 移除"/Game"前缀，获取相对路径
		String relativePath = workingPath.substr(5); // "/Game" = 5个字符
		
		// 将正斜杠替换为系统路径分隔符（Windows使用反斜杠）
		for (auto& ch : relativePath)
		{
			if (ch == '/')
			{
				ch = '\\';
			}
		}
		
		// 获取项目根路径
		String projectRoot = FileModule::GetProjectRoot();
		
		// 构建完整路径
		String fullPath = projectRoot + "\\Content" + relativePath;
		
		// 添加文件扩展名
		if (!extension.empty())
		{
			if (extension[0] == '.')
			{
				fullPath += extension;
			}
			else
			{
				fullPath += "." + extension;
			}
		}
		
		return fullPath;
	}

	bool PackageModule::CheckUniqueSoftPath(String& softPath)
	{
		// check duplicate name
		String checkPath = softPath;
		int suffix = 1;
		bool bUnique = true;
#if WITH_EDITOR
		while (GetModule()->SoftPathToGuidMap.contains(checkPath))
		{
			bUnique = false;
			checkPath = softPath + "_" + std::to_string(suffix);
			suffix++;
		}
		if (!bUnique)
		{
			softPath = checkPath;
		}
#endif
		return bUnique;
	}

#if WITH_EDITOR
	// Get last_write_time and file size as int64 values.
	static std::pair<int64, int64> GetSrcFileInfo(const String& path)
	{
		std::error_code ec;
		auto ftime = std::filesystem::last_write_time(path, ec);
		int64 mtime = ec ? 0 : static_cast<int64>(ftime.time_since_epoch().count());
		int64 fsize = 0;
		if (!ec)
		{
			auto sz = std::filesystem::file_size(path, ec);
			fsize = ec ? 0 : static_cast<int64>(sz);
		}
		return { mtime, fsize };
	}

	static bool ImportAssetIntoPackage(
		const String& srcPath,
		const String& destPath,
		const TGuid& pakGuid,
		Package* newPackage,
		PackageEntry* newEntry,
		const TGuid& existingResGuid,
		bool bCheckUniqueName)
	{
		const String fileExtension = FileModule::GetFileExtension(srcPath);

		if (fileExtension == "fbx")
		{
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(
				srcPath,
				aiProcess_Triangulate |
				aiProcess_FlipUVs |
				aiProcess_GenSmoothNormals |
				aiProcess_CalcTangentSpace |
				aiProcess_OptimizeMeshes
			);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				TAssertf(false, "ImportAssetIntoPackage: Assimp failed on \"%s\": %s", srcPath.c_str(), importer.GetErrorString());
				return false;
			}

			LOG("The model contains %d meshes", scene->mNumMeshes);
			const auto newStaticMesh = new StaticMesh();
			for (unsigned int i = 0; i < scene->mNumMeshes; i++)
			{
				const aiMesh* mesh = scene->mMeshes[i];
				LOG("Mesh %d: %d vertices", i, mesh->mNumVertices);
				const auto subMesh = new (TMemory::Malloc<SubMesh>()) SubMesh(i);
				auto vertices = GenerateVertexBuffer(mesh);
				auto indices = GenerateIndicesBuffer(mesh);
				AABB bb = AABB(
					TVector3f(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z),
					TVector3f(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z)
				);
				subMesh->SetContents(vertices, indices, bb);
				newStaticMesh->GetSubMeshes().push_back(subMesh);
			}

			if (existingResGuid.IsValid())
			{
				newStaticMesh->SetGuid(existingResGuid);
			}

			String resourceSuffix = scene->mName.length > 0 ? scene->mName.C_Str() : FileModule::GetFileName(destPath);
			String resourceName = PackageModule::CovertFullPathToSoftPath(destPath, resourceSuffix);
			if (bCheckUniqueName)
			{
				PackageModule::CheckUniqueSoftPath(resourceName);
			}
			newStaticMesh->SetResourceName(resourceName);
			newStaticMesh->SetOuter(newPackage);
			TGuid meshGuid = newStaticMesh->GetGUID();
			PackageModule::AddResourceToPackage(meshGuid, pakGuid);
			newEntry->Dependencies.insert({meshGuid, {}});
			newPackage->AddResource(newStaticMesh);

			LOG("Imported fbx: %s", resourceName.c_str());
		}
		else if (fileExtension == "png" || fileExtension == "tga")
		{
			int width, height, channels;
			unsigned char* data = stbi_load(srcPath.c_str(), &width, &height, &channels, 0);
			if (!data) [[unlikely]]
			{
				TAssertf(false, "ImportAssetIntoPackage: Failed to load image \"%s\".", srcPath.c_str());
				return false;
			}

			const auto newTexture = new Texture2D(data, width, height, channels);
			stbi_image_free(data);

			if (existingResGuid.IsValid())
			{
				newTexture->SetGuid(existingResGuid);
			}

			const String resourceName = PackageModule::CovertFullPathToSoftPath(destPath, FileModule::GetFileName(destPath));
			newTexture->SetResourceName(resourceName);
			newTexture->SetOuter(newPackage);
			TGuid texGuid = newTexture->GetGUID();
			PackageModule::AddResourceToPackage(texGuid, pakGuid);
			newEntry->Dependencies.insert({texGuid, {}});
			newPackage->AddResource(newTexture);

			LOG("Imported %s: %s", fileExtension.c_str(), resourceName.c_str());
		}
		else if (fileExtension == "mat")
		{
			using namespace rapidjson;

			String jsonString;
			if (!FileModule::LoadFileToString(srcPath, jsonString))
			{
				TAssertf(false, "ImportAssetIntoPackage: Failed to load material \"%s\".", srcPath.c_str());
				return false;
			}

			Document document;
			document.Parse(jsonString.c_str());
			if (document.HasParseError()) [[unlikely]]
			{
				TAssertf(false, "ImportAssetIntoPackage: Failed to parse material JSON \"%s\".", srcPath.c_str());
				return false;
			}

			GameMaterial* material = new GameMaterial();
			if (document.HasMember("ShaderName") && document["ShaderName"].IsString())
			{
				material->SetShaderArchive(NameHandle(document["ShaderName"].GetString()));
				material->ResetDefaultParameters();
			}
			else
			{
				TAssertf(false, "ImportAssetIntoPackage: Material missing ShaderName \"%s\".", srcPath.c_str());
				delete material;
				return false;
			}

			if (document.HasMember("Parameters") && document["Parameters"].IsObject())
			{
				const auto& parameters = document["Parameters"].GetObject();
				for (auto itr = parameters.MemberBegin(); itr != parameters.MemberEnd(); ++itr)
				{
					if (!itr->value.IsString())
					{
						continue;
					}
					String paramName = itr->name.GetString();
					String paramSoftPath = itr->value.GetString();
					TGuid guid = PackageModule::GetGuidBySoftPath(NameHandle(paramSoftPath));
					if (!guid.IsValid()) [[unlikely]]
					{
						TAssertf(false, "ImportAssetIntoPackage: Texture parameter \"%s\" path \"%s\" not found in material \"%s\".",
							paramName.c_str(), paramSoftPath.c_str(), srcPath.c_str());
						delete material;
						return false;
					}
					material->SetTextureParameter(NameHandle(paramName), guid);
				}
			}

			if (existingResGuid.IsValid())
			{
				material->SetGuid(existingResGuid);
			}

			String matSoftPath = PackageModule::CovertFullPathToSoftPath(destPath, FileModule::GetFileName(destPath));
			if (bCheckUniqueName)
			{
				PackageModule::CheckUniqueSoftPath(matSoftPath);
			}
			material->SetResourceName(matSoftPath);
			material->SetOuter(newPackage);
			TGuid materialGuid = material->GetGUID();
			PackageModule::AddResourceToPackage(materialGuid, pakGuid);
			newEntry->Dependencies.insert({materialGuid, {}});
			newPackage->AddResource(material);

			LOG("Imported material: %s", matSoftPath.c_str());
		}
		else
		{
			TAssertf(false, "ImportAssetIntoPackage: Unsupported file extension \"%s\" for \"%s\".", fileExtension.c_str(), srcPath.c_str());
			return false;
		}

		return true;
	}

	bool PackageModule::Import(const String& srcPath, const String& destPath)
	{
		const String packName = CovertFullPathToSoftPath(destPath);
		auto* newPackage = new Package(packName, {});
		TGuid pakGuid = newPackage->GetGUID();
		PackageEntry* newEntry = AddPackageEntry(pakGuid, packName);
		AddResourceToPackage(pakGuid, pakGuid);

		auto [mtime, fsize] = GetSrcFileInfo(srcPath);
		newPackage->SetSourceFileInfo(mtime, fsize);

		if (!ImportAssetIntoPackage(srcPath, destPath, pakGuid, newPackage, newEntry, {}, /*bCheckUniqueName=*/true))
		{
			return false;
		}

		if (!GetModule()->SavePackage(newPackage))
		{
			return false;
		}

		LOG("Import: saved package \"%s\".", packName.c_str());
		GetModule()->RegisterPackageSoftPathToGUID(newPackage);
		newEntry->SrcFileLastWriteTime = mtime;
		newEntry->SrcFileSize = fsize;

		// Sync GameResource::Dependencies to PackageEntry::Dependencies.
		for (auto res : newPackage->GetPackageObjects())
		{
			const TGuid resGuid = res->GetGUID();
			newEntry->Dependencies[resGuid] = res->GetDependencies();
		}

		return true;
	}

	bool PackageModule::ReImport(const String& srcPath, const String& destPath, const TGuid& existingPakGuid)
	{
		PackageModule* mod = GetModule();

		// Find existing package entry and extract the resource GUID to reuse.
		auto pakIt = mod->PackageMap.find(existingPakGuid);
		if (pakIt == mod->PackageMap.end()) [[unlikely]]
		{
			TAssertf(false, "ReImport: PackageEntry not found for GUID \"%s\", srcPath=\"%s\".",
				existingPakGuid.ToString().c_str(), srcPath.c_str());
			return false;
		}
		TGuid existingResGuid;
		for (auto& [resGuid, _] : pakIt->second->Dependencies)
		{
			if (resGuid != existingPakGuid)
			{
				existingResGuid = resGuid;
				break;
			}
		}

		auto [mtime, fsize] = GetSrcFileInfo(srcPath);

		// Remove the stale entry so AddPackageEntry can re-insert with the same GUID.
		TArray<TGuid> resToRemove;
		for (auto& [resGuid, pakGuid] : mod->ResourceToPackage)
		{
			if (pakGuid == existingPakGuid)
			{
				resToRemove.push_back(resGuid);
			}
		}
		for (const TGuid& g : resToRemove)
		{
			mod->ResourceToPackage.erase(g);
		}
		mod->PackageMap.erase(existingPakGuid);

		const String packName = CovertFullPathToSoftPath(destPath);
		auto* newPackage = new Package(packName, existingPakGuid);
		newPackage->SetSourceFileInfo(mtime, fsize);

		PackageEntry* newEntry = AddPackageEntry(existingPakGuid, packName);
		AddResourceToPackage(existingPakGuid, existingPakGuid);

		if (!ImportAssetIntoPackage(srcPath, destPath, existingPakGuid, newPackage, newEntry, existingResGuid, /*bCheckUniqueName=*/false))
		{
			return false;
		}

		if (!GetModule()->SavePackage(newPackage))
		{
			return false;
		}

		LOG("ReImport: saved package \"%s\".", packName.c_str());
		GetModule()->RegisterPackageSoftPathToGUID(newPackage);
		newEntry->SrcFileLastWriteTime = mtime;
		newEntry->SrcFileSize = fsize;

		// Sync GameResource::Dependencies to PackageEntry::Dependencies.
		for (auto res : newPackage->GetPackageObjects())
		{
			const TGuid resGuid = res->GetGUID();
			newEntry->Dependencies[resGuid] = res->GetDependencies();
		}

		return true;
	}

	bool PackageModule::ImportTmap(const String& srcPath, const String& destPath)
	{
		using namespace rapidjson;

		String jsonString;
		if (!FileModule::LoadFileToString(srcPath, jsonString))
		{
			TAssertf(false, "ImportTmap: Failed to load tmap file \"%s\".", srcPath.c_str());
			return false;
		}

		Document document;
		document.Parse(jsonString.c_str());
		if (document.HasParseError()) [[unlikely]]
		{
			TAssertf(false, "ImportTmap: Failed to parse tmap JSON \"%s\".", srcPath.c_str());
			return false;
		}

		// Recursively replace soft-path strings with GUIDs.
		PackageModule* mod = GetModule();
		std::function<void(Value&, Document::AllocatorType&)> replaceGuids =
			[&](Value& val, Document::AllocatorType& allocator)
		{
			if (val.IsString())
			{
				String str = val.GetString();
				if (str.size() >= 5 && str.substr(0, 5) == "/Game")
				{
					auto it = mod->SoftPathToGuidMap.find(NameHandle(str));
					if (it != mod->SoftPathToGuidMap.end())
					{
						String guidStr = it->second.ToString();
						val.SetString(guidStr.c_str(), static_cast<rapidjson::SizeType>(guidStr.size()), allocator);
					}
					else
					{
						LOG("ImportTmap: soft path \"%s\" not found in SoftPathToGuidMap, left as-is.", str.c_str());
					}
				}
			}
			else if (val.IsObject())
			{
				for (auto itr = val.MemberBegin(); itr != val.MemberEnd(); ++itr)
				{
					replaceGuids(itr->value, allocator);
				}
			}
			else if (val.IsArray())
			{
				for (auto& elem : val.GetArray())
				{
					replaceGuids(elem, allocator);
				}
			}
		};
		replaceGuids(document, document.GetAllocator());

		StringBuffer buffer;
		PrettyWriter<StringBuffer> writer(buffer);
		writer.SetIndent(' ', 4);
		document.Accept(writer);

		// Ensure the directory exists before writing.
		std::filesystem::create_directories(std::filesystem::path(destPath).parent_path());

		String outJson = buffer.GetString();
		if (!FileModule::SaveFileFromString(destPath, outJson))
		{
			TAssertf(false, "ImportTmap: Failed to save tmap file \"%s\".", destPath.c_str());
			return false;
		}

		LOG("ImportTmap: wrote \"%s\".", destPath.c_str());
		return true;
	}

	void PackageModule::ImportAll(bool bForce)
	{
		const String srcRoot = FileModule::GetProjectRoot() + "\\Resource\\";
		const String destRoot = FileModule::GetResourceContentRoot();

		TArray<String> allFiles;
		FileModule::TraverseFileFromFolder(srcRoot, allFiles);

		// Classify files by extension.
		TArray<String> fbxFiles, texFiles, matFiles, tmapFiles;
		for (const String& fileName : allFiles)
		{
			String ext = FileModule::GetFileExtension(fileName);
			if (ext == "fbx")
				fbxFiles.push_back(fileName);
			else if (ext == "png" || ext == "tga")
				texFiles.push_back(fileName);
			else if (ext == "mat")
				matFiles.push_back(fileName);
			else if (ext == "tmap")
				tmapFiles.push_back(fileName);
		}

		// Look up the existing PackageEntry for a dest path via SoftPathToGuidMap (O(1)).
		auto findExistingEntry = [&](const String& destPath) -> PackageEntry*
		{
			NameHandle destSoftPath(CovertFullPathToSoftPath(destPath));
			auto it = SoftPathToGuidMap.find(destSoftPath);
			if (it == SoftPathToGuidMap.end())
			{
				return nullptr;
			}
			auto pakIt = PackageMap.find(it->second);
			return pakIt != PackageMap.end() ? pakIt->second : nullptr;
		};

		auto doImport = [&](const String& srcPath)
		{
			String relativePath = srcPath.substr(srcRoot.length());
			String destPath = destRoot + relativePath;
			destPath = FileModule::SwitchFileExtension(destPath, "tasset");

			PackageEntry* existingEntry = findExistingEntry(destPath);
			if (existingEntry != nullptr)
			{
				auto [curMtime, curSize] = GetSrcFileInfo(srcPath);
				if (!bForce && existingEntry->SrcFileLastWriteTime == curMtime && existingEntry->SrcFileSize == curSize)
				{
					LOG("ImportAll: skip (unchanged) \"%s\".", srcPath.c_str());
					return;
				}
				ReImport(srcPath, destPath, existingEntry->Guid);
				return;
			}

			Import(srcPath, destPath);
		};

		// Meshes and textures first (no cross-asset dependencies).
		for (const String& f : fbxFiles) doImport(f);
		for (const String& f : texFiles) doImport(f);

		// Materials depend on textures, so import them after.
		for (const String& f : matFiles) doImport(f);

		// Maps: always re-process (text replacement, negligible cost).
		for (const String& srcPath : tmapFiles)
		{
			String relativePath = srcPath.substr(srcRoot.length());
			String destPath = destRoot + relativePath;
			ImportTmap(srcPath, destPath);
		}
	}

	void PackageModule::RegisterPackageSoftPathToGUID(Package* package)
	{
		TGuid pakGuid = package->GetGUID();
		SoftPathToGuidMap.emplace(package->GetPackageName(), pakGuid);

		const TArray<GameResource*> objects = package->GetPackageObjects();
		for (auto obj : objects)
		{
			TGuid objGuid = obj->GetGUID();
			SoftPathToGuidMap.emplace(obj->GetResourceName(), objGuid);
		}
	}
#endif
}
