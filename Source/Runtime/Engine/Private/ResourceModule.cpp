#pragma optimize("", off) 
#include "ResourceModule.h"
#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"		// 定义 stb_image 实现
#include <assimp/Importer.hpp>      // 导入接口
#include <assimp/scene.h>           // 场景数据（模型、材质、动画）
#include <assimp/postprocess.h>     // 后处理选项（如三角化、优化）

#include "Mesh.h"
#include "Package.h"
#include "Texture.h"
#include "Container/ReflectiveContainer.h"
#include "FileSystem/FileModule.h"
#include "Vector.h"

namespace Thunder
{
	IMPLEMENT_MODULE(Resource, ResourceModule)

	static TReflectiveContainerRef GenerateVertexBuffer(const aiMesh* mesh)
	{
		auto vertexBuffer = MakeRefCount<ReflectiveContainer>();
		vertexBuffer->SetDataNum(mesh->mNumVertices);
		// 注册顶点数据 layout
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
			vertexBuffer->AddComponent<TVector3f>("Bitangent");
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
		// 提取顶点数据
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
			for (size_t v = 0, offset = vertexBuffer->GetComponentOffset("Bitangent"); v < mesh->mNumVertices; v++, offset += compStride)
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

	void ResourceModule::StartUp()
	{
		const auto contentDict = FileModule::GetProjectRoot() + "\\Content\\";
		TArray<String> fileNames;
		FileModule::TraverseFileFromFolder(contentDict, fileNames);
		for (const auto& fileName : fileNames)
		{
			if (FileModule::GetFileExtension(fileName) == "tasset")
			{
				// only load guid
				TGuid guid;
				if (Package::LoadOnlyGuid(fileName, guid))
				{
					String softPath = CovertFullPathToSoftPath(fileName);
					ResourcePathMap.emplace(guid, softPath);
				}
				else
				{
					LOG("Failed to load package guid from %s", fileName.c_str());
				}
			}
		}
	}

	String ResourceModule::CovertFullPathToSoftPath(const String& fullPath, const String& resourceName)
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

	String ResourceModule::ConvertSoftPathToFullPath(const String& softPath, const String& extension)
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

	bool ResourceModule::Import(const String& srcPath, const String& destPath)
	{
		// 获取源文件类型
		String fileExtension = FileModule::GetFileExtension(srcPath);
		if (fileExtension == "fbx")
		{
			// 1. 创建 Importer 对象
			Assimp::Importer importer;

			// 2. 加载模型文件（FBX/OBJ/glTF）
			const aiScene* scene = importer.ReadFile(
				srcPath,
				aiProcess_Triangulate |          // 三角化
				aiProcess_FlipUVs |              // 翻转 UV（可选）
				aiProcess_GenNormals |           // 生成法线（如果没有）
				aiProcess_OptimizeMeshes        // 优化网格
			);

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				LOG("Assimp import error: %s", importer.GetErrorString());
				return false;
			}

			// 3. 遍历模型数据
			LOG("The model contains %d meshes\n", scene->mNumMeshes);

			const auto newStaticMesh = new StaticMesh();
			for (unsigned int i = 0; i < scene->mNumMeshes; i++)
			{
				const aiMesh* mesh = scene->mMeshes[i];
				LOG("Mesh %d: %d vertices\n", i, mesh->mNumVertices);

				const auto subMesh = MakeRefCount<SubMesh>();
				subMesh->Vertices = GenerateVertexBuffer(mesh);
				subMesh->Indices = GenerateIndicesBuffer(mesh);
				subMesh->BoundingBox = AABB(
					TVector3f(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z),
					TVector3f(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z)
				);
				newStaticMesh->SubMeshes.push_back(subMesh);
			}

			// 4. 注册到资源管理器
			const String resourceName = CovertFullPathToSoftPath(destPath, scene->mName.C_Str());
			newStaticMesh->SetResourceName(resourceName);
			TGuid meshGuid = newStaticMesh->GetGUID();
			GetModule()->LoadedResources.emplace(meshGuid, newStaticMesh);
			GetModule()->LoadedResourcesByPath.emplace(resourceName, meshGuid);

			// 5. 保存 package 文件
			const String pacName = CovertFullPathToSoftPath(destPath);
			auto* newPackage = new Package(pacName); //需要区分这个path，有虚拟路径和绝对路径，暂时都用绝对路径
			newPackage->AddResource(newStaticMesh);
			
			if (GetModule()->SavePackage(newPackage, destPath))
			{
				TGuid pakGuid = newPackage->GetGUID();
				GetModule()->LoadedResources.emplace(pakGuid, newPackage);
				GetModule()->LoadedResourcesByPath.emplace(pacName, meshGuid);
				GetModule()->ResourcePathMap.emplace(pakGuid, pacName);
				return true;
			}
		}
		if (fileExtension == "png" || fileExtension == "tga")
		{
			int width, height, channels;
			unsigned char* data = stbi_load(srcPath.c_str(), &width, &height, &channels, 0);
			if (data)
			{
				const auto newTexture = new Texture2D(data, width, height, channels);
				stbi_image_free(data); // data在Texture2D中拷贝了一份，stbi管理的数据原地释放
				
				// 注册到资源管理器
				const String resourceName = CovertFullPathToSoftPath(destPath, FileModule::GetFileName(destPath));
				newTexture->SetResourceName(resourceName);
				TGuid meshGuid = newTexture->GetGUID();
				GetModule()->LoadedResources.emplace(meshGuid, newTexture);
				GetModule()->LoadedResourcesByPath.emplace(resourceName, meshGuid);

				// 保存 package 文件
				const String pacName = CovertFullPathToSoftPath(destPath);
				auto* newPackage = new Package(pacName); //需要区分这个path，有虚拟路径和绝对路径，暂时都用绝对路径
				newPackage->AddResource(newTexture);
			
				if (GetModule()->SavePackage(newPackage, destPath))
				{
					TGuid pakGuid = newPackage->GetGUID();
					GetModule()->LoadedResources.emplace(pakGuid, newPackage);
					GetModule()->LoadedResourcesByPath.emplace(pacName, meshGuid);
					GetModule()->ResourcePathMap.emplace(pakGuid, pacName);
					return true;
				}
			}
			else
			{
				// 处理加载失败的情况
				std::cerr << "Failed to load image: " << srcPath << std::endl;
				return false;
			}
		}
		return false;
	}

	void ResourceModule::ImportAll(bool bForce)
	{
		if (bForce)
		{
			auto srcDict = FileModule::GetProjectRoot() + "\\Resource\\";
			auto destDict = FileModule::GetProjectRoot() + "\\Content\\";
			TArray<String> fileNames;
			FileModule::TraverseFileFromFolder(srcDict, fileNames);
			for (const auto& fileName : fileNames)
			{
				String relativePath = fileName.substr(srcDict.length());
				String destPath = destDict + relativePath;
				destPath = FileModule::SwitchFileExtension(destPath, "tasset");
				Import(fileName, destPath);
			}
		}
		else
		{
			//todo 增量导入
		}
	}
}