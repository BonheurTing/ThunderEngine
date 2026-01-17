#pragma optimize("", off)
#include "RenderMesh.h"
#include "IDynamicRHI.h"
#include "RenderTranslator.h"
#include "Concurrent/TaskScheduler.h"
#include "Container/ReflectiveContainer.h"

namespace Thunder
{
    bool SubMesh::GetVertexDeclaration(TArray<RHIVertexElement>& outDeclarations) const
    {
        auto const& container = Vertices;
        outDeclarations.clear();

        if (!container)
        {
            TAssert(false, "Failed to get vertex declaration, vertex buffer is empty.");
            return false;
        }

        size_t componentCount = container->GetComponentCount();
        for (size_t i = 0; i < componentCount; ++i)
        {
            const TypeComponent* componentInfo = container->GetComponentInfo(i);
            if (!componentInfo)
            {
                TAssert(false, "Empty component found in vertex buffer.");
                continue;
            }

            ERHIVertexInputSemantic semantic = RenderTranslator::GetVertexSemanticFromName(componentInfo->Name);
            if (semantic == ERHIVertexInputSemantic::Unknown)
            {
                TAssert(false, "Invalid vertex semantic found in vertex buffer.");
                continue;
            }

            RHIFormat format = RenderTranslator::GetRHIFormatFromTypeKind(componentInfo->Kind);
            if (format == RHIFormat::UNKNOWN)
            {
                TAssert(false, "Invalid vertex format found in vertex buffer.");
                continue;
            }

            // Get semantic index.
            uint8 semanticIndex = 0;
            String name = componentInfo->Name;
            for (size_t j = 0; j < name.size(); ++j)
            {
                if (isdigit(name[j]))
                {
                    semanticIndex = static_cast<uint8>(std::stoi(name.substr(j)));
                    break;
                }
            }

            outDeclarations.emplace_back(
                semantic,
                semanticIndex,
                format,
                0, // InputSlot.
                static_cast<uint16>(componentInfo->Offset),
                false // PerInstanceData.
            );
        }

        return true;
    }

    bool SubMesh::GetVertexDeclaration(RHIVertexDeclarationDescriptor& outDeclarations) const
    {
        return GetVertexDeclaration(outDeclarations.Elements);
    }

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

    RenderStaticMesh::RenderStaticMesh(const TArray<SubMesh*>& subMeshes)
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
