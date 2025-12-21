#pragma optimize("", off)
#include "Mesh.h"
#include "Concurrent/TaskScheduler.h"

namespace Thunder
{
	IMesh::~IMesh()
	{
		ReleaseResource();
	}

	void IMesh::OnResourceLoaded()
	{
		LOG("Mesh loaded: %s", GetResourceName().c_str());

		ReleaseResource();
		RenderMesh* newResource = CreateResource_GameThread(); //纯虚函数
		SetResource(newResource);
		InitResource();
	}

	void IMesh::SetResource(RenderMesh* resource)
	{
		GRenderScheduler->PushTask([this, resource]()
		{
			this->MeshResource = resource;
		});
	}

	void IMesh::ReleaseResource()
	{
		if (MeshResource)
		{
			GRenderScheduler->PushTask([resource = this->MeshResource]()
			{
				resource->ReleaseResource();
			});
		}
	}

	void IMesh::InitResource()
	{
		GRenderScheduler->PushTask([resource = MeshResource.Get()]()
		{
			if (resource)
			{
				resource->InitResource();
			}
		});
	}

	RenderMesh* StaticMesh::CreateResource_GameThread()
	{
		auto renderMesh = new RenderStaticMesh(SubMeshes);
		return renderMesh;
	}

	StaticMesh::~StaticMesh()
	{
		for (auto subMesh : SubMeshes)
		{
			if (subMesh)
			{
				TMemory::Destroy(subMesh);
			}
		}
	}

	void StaticMesh::Serialize(MemoryWriter& archive)
	{
		GameResource::Serialize(archive);
		const auto subMeshCount = static_cast<uint32>(SubMeshes.size());
		archive << subMeshCount;
		for(const auto& subMesh : SubMeshes)
		{
			subMesh->Vertices.Get()->Serialize(archive);
			subMesh->Indices.Get()->Serialize(archive);
			subMesh->BoundingBox.Serialize(archive);
		}
	}

	void StaticMesh::DeSerialize(MemoryReader& archive)
	{
		GameResource::DeSerialize(archive);

		uint32 subMeshCount;
		archive >> subMeshCount;

		SubMeshes.clear();
		SubMeshes.resize(subMeshCount);

		for (uint32 i = 0; i < subMeshCount; ++i)
		{
			const auto subMesh = new (TMemory::Malloc<SubMesh>()) SubMesh();

			// 反序列化顶点缓冲区
			subMesh->Vertices = MakeRefCount<ReflectiveContainer>();
			subMesh->Vertices->DeSerialize(archive);

			// 反序列化索引缓冲区
			subMesh->Indices = MakeRefCount<ReflectiveContainer>();
			subMesh->Indices->DeSerialize(archive);

			// 反序列化包围盒
			subMesh->BoundingBox.DeSerialize(archive);

			SubMeshes[i] = subMesh;
		}
	}

	void StaticMesh::AddMaterial(IMaterial* material)
	{
		DefaultMaterials.push_back(material);
		AddDependency(material->GetGUID());
	}
}
