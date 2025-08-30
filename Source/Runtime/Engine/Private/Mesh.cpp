#include "Mesh.h"

namespace Thunder
{
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
			const auto subMesh = MakeRefCount<SubMesh>();
			
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

}