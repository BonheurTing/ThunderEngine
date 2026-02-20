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
		auto renderMesh = new RenderMesh(SubMeshes);
		return renderMesh;
	}

	StaticMesh::~StaticMesh()
	{
	}

	void StaticMesh::Serialize(MemoryWriter& archive)
	{
		GameResource::Serialize(archive);
		const auto subMeshCount = static_cast<uint32>(SubMeshes.size());
		archive << subMeshCount;
		for(const auto& subMesh : SubMeshes)
		{
			subMesh->GetVertices()->Serialize(archive);
			subMesh->GetIndices()->Serialize(archive);
			subMesh->GetBoundingBox().Serialize(archive);
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
			const auto subMesh = new (TMemory::Malloc<SubMesh>()) SubMesh(i);

			auto vertices = MakeRefCount<ReflectiveContainer>();
			vertices->DeSerialize(archive);

			auto indices = MakeRefCount<ReflectiveContainer>();
			indices->DeSerialize(archive);

			AABB boundingBox;
			boundingBox.DeSerialize(archive);

			subMesh->SetContents(vertices, indices, boundingBox);
			SubMeshes[i] = subMesh;
		}
	}

	void StaticMesh::AddMaterial(IMaterial* material)
	{
		DefaultMaterials.push_back(material);
		AddDependency(material->GetGUID());
	}
}
