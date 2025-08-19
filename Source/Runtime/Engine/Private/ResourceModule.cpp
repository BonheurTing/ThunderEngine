#pragma optimize("", off) 
#include "ResourceModule.h"
#include "External/stb_image.h"
#include <assimp/Importer.hpp>      // 导入接口
#include <assimp/scene.h>           // 场景数据（模型、材质、动画）
#include <assimp/postprocess.h>     // 后处理选项（如三角化、优化）

#include "Mesh.h"
#include "Package.h"
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

	String ResourceModule::CovertFullPathToContentPath(const String& fullPath)
	{
		size_t contentPos = fullPath.find_first_of("Content");
		if (contentPos == std::string::npos) {
			return ""; // 如果没有找到"Content"，返回空字符串
		}

		std::string result = fullPath.substr(contentPos);

		const size_t lastDotPos = result.find_last_of('.');
		if (lastDotPos != std::string::npos) {
			result = result.substr(0, lastDotPos);
		}

		return result;
	}

	String ResourceModule::ConvertContentPathToFullPath(const String& contentPath, const String& extension)
	{
		return FileModule::GetProjectRoot() + "\\" + contentPath + extension;
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
			String resourceVirtualPath = CovertFullPathToContentPath(destPath);
			auto* newPackage = new Package(resourceVirtualPath); //需要区分这个path，有虚拟路径和绝对路径，暂时都用绝对路径
			newPackage->AddResource(newStaticMesh);
			TGuid guid = newStaticMesh->GetGUID();
			GetModule()->LoadedResources.emplace(guid, newStaticMesh);
			GetModule()->LoadedResourcesByPath.emplace(resourceVirtualPath, guid);

			// 5. 保存 package 文件
			return GetModule()->SavePackage(newPackage, destPath);
		}
		if (fileExtension == "png" || fileExtension == "tga")
		{
			int width, height, channels;
			unsigned char* data;// = stbi_load(srcPath.c_str(), &width, &height, &channels, 0);
			if (data)
			{
				/*// 创建目标文件夹
				FileModule::CreateDirectory(FileModule::GetFilePath(destPath));
				// 保存图片数据到目标路径
				TRefCountPtr<NativeFile> file = FileModule::GetFileSystem("Content")->Open(destPath, EFileMode::Write);
				file->Write(data, width * height * channels);*/
				//stbi_image_free(data);
			}
			else
			{
				// 处理加载失败的情况
				std::cerr << "Failed to load image: " << srcPath << std::endl;
			}
			return true;
		}
		std::cerr << "Unsupported file type: " << fileExtension << std::endl;
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
				Import(fileName, destPath);
			}
		}
		else
		{
			//todo 增量导入
		}
	}
}
#pragma optimize("", on) 