#pragma optimize("", off)
#include "RenderMesh.h"
#include "IDynamicRHI.h"
#include "Concurrent/TaskScheduler.h"
#include "Container/ReflectiveContainer.h"

namespace Thunder
{
    void RenderMesh::InitRHI()
    {
        CreateMesh_RenderThread();
    }

    void RenderMesh::ReleaseRHI()
    {
        for (auto vertex : VBs)
        {
            vertex.SafeRelease();
        }
        for (auto index : IBs)
        {
            index.SafeRelease();
        }
    }

    RenderStaticMesh::RenderStaticMesh(const TArray<TSubMeshRef>& subMeshes)
    {
        NumSubMeshes = static_cast<uint32>(subMeshes.size());
        for (auto& subMesh : subMeshes)
        {
            RawVerticesData.push_back(subMesh->Vertices);
            RawIndicesData.push_back(subMesh->Indices);
        }
        bDynamic = false;
    }

    void RenderStaticMesh::CreateMesh_RenderThread()
    {
        //check(bRenderThread)
        TAssert(bDynamic == false);
        VBs.resize(NumSubMeshes);
        IBs.resize(NumSubMeshes);
        for (uint32 i = 0; i < NumSubMeshes; i++)
        {
            // vb
            {
                const auto& vertices = RawVerticesData[i];
                uint32 vertexStride = static_cast<uint32>(vertices->GetStride());

                RHIVertexBufferRef renderThreadVertexBuffer = RHICreateVertexBuffer(
                    static_cast<uint32>(vertices->GetTotalSize()),
                    vertexStride,
                    EBufferCreateFlags::Static
                );
                VBs[i] = renderThreadVertexBuffer;
            }
            // ib
            {
                const auto& indices = RawIndicesData[i];
                uint32 indexDataSize = static_cast<uint32>(indices->GetTotalSize());
                ERHIIndexBufferType indexType = ERHIIndexBufferType::Uint32; //temp

                RHIIndexBufferRef renderThreadIndexBuffer = RHICreateIndexBuffer(
                    indexDataSize,
                    indexType,
                    EBufferCreateFlags::Static
                );
                IBs[i] = renderThreadIndexBuffer;
            }
        }

        GRHIScheduler->PushTask([num = NumSubMeshes, binVertex = RawVerticesData, binIndex = RawIndicesData, rhiResVB = VBs, rhiResIB = IBs, bAsync = isDoubleBuffered() || (!isDynamic())]()
        {
            for (uint32 i = 0; i < num; i++)
            {
                rhiResVB[i]->SetBinaryData(binVertex[i]);
                rhiResIB[i]->SetBinaryData(binIndex[i]);
                if (bAsync)
                {
                    GRHIUpdateAsyncQueue.push_back(rhiResVB[i]);
                    GRHIUpdateAsyncQueue.push_back(rhiResIB[i]);
                }
                else
                {
                    GRHIUpdateSyncQueue.push_back(rhiResVB[i]);
                    GRHIUpdateSyncQueue.push_back(rhiResIB[i]);
                }
            }
        });
    }
}
