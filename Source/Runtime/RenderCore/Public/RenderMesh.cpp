#pragma optimize("", off)
#include "RenderMesh.h"
#include "IDynamicRHI.h"
#include "RenderTranslator.h"
#include "Concurrent/TaskScheduler.h"
#include "Container/ReflectiveContainer.h"

namespace Thunder
{
    ProceduralGeometryManager* GProceduralGeometryManager = nullptr;
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

    ProceduralGeometryManager::~ProceduralGeometryManager()
    {
        for (auto& subMesh : SubMeshes | std::views::values)
        {
            if (subMesh)
            {
                delete subMesh;
            }
        }
        SubMeshes.clear();
    }

    void ProceduralGeometryManager::InitBasicGeometrySubMeshes()
    {
        // Init cube (24 vertices for proper per-face UVs)
        {
            TArray<TVector4f> positions{
                // Top face (Y+)
                { -.5f, .5f, .5f, 1.f },
                { -.5f, .5f, -.5f, 1.f },
                { .5f, .5f, -.5f, 1.f },
                { .5f, .5f,  .5f, 1.f },
                // Bottom face (Y-)
                { -.5f, -.5f,  .5f, 1.f },
                { .5f, -.5f,  .5f, 1.f },
                { .5f, -.5f, -.5f, 1.f },
                { -.5f, -.5f, -.5f, 1.f },
                // Front face (Z+)
                { -.5f, -.5f, .5f, 1.f },
                { -.5f,  .5f, .5f, 1.f },
                { .5f,  .5f, .5f, 1.f },
                { .5f, -.5f, .5f, 1.f },
                // Back face (Z-)
                { .5f, -.5f, -.5f, 1.f },
                { .5f,  .5f, -.5f, 1.f },
                { -.5f,  .5f, -.5f, 1.f },
                { -.5f, -.5f, -.5f, 1.f },
                // Right face (X+)
                { .5f, -.5f,  .5f, 1.f },
                { .5f,  .5f,  .5f, 1.f }, 
                { .5f,  .5f, -.5f, 1.f }, 
                { .5f, -.5f, -.5f, 1.f },
                // Left face (X-)
                { -.5f, -.5f, -.5f, 1.f }, 
                { -.5f,  .5f, -.5f, 1.f }, 
                { -.5f,  .5f,  .5f, 1.f }, 
                { -.5f, -.5f,  .5f, 1.f },
            };
            TArray<TVector2f> uvs{
                // Each face: bottom-left, top-left, top-right, bottom-right
                { 0.f, 0.f }, { 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 0.f },
                { 0.f, 0.f }, { 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 0.f },
                { 0.f, 0.f }, { 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 0.f },
                { 0.f, 0.f }, { 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 0.f },
                { 0.f, 0.f }, { 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 0.f },
                { 0.f, 0.f }, { 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 0.f },
            };
            TReflectiveContainerRef vertexBuffer = new ReflectiveContainer();
            vertexBuffer->AddComponent<TVector4f>("Position");
            vertexBuffer->AddComponent<TVector2f>("UV0");
            vertexBuffer->SetDataNum(static_cast<size_t>(positions.size()));
            vertexBuffer->Initialize();
            vertexBuffer->SetComponentData("Position", positions);
            vertexBuffer->SetComponentData("UV0", uvs);

            TArray<uint32> indices{
                 0,  1,  2,  0,  2,  3, // Top
                 4,  5,  6,  4,  6,  7, // Bottom
                 8,  9, 10,  8, 10, 11, // Front
                12, 13, 14, 12, 14, 15, // Back
                16, 17, 18, 16, 18, 19, // Right
                20, 21, 22, 20, 22, 23, // Left
            };
            TReflectiveContainerRef indexBuffer = new ReflectiveContainer();
            indexBuffer->AddComponent<uint32>("Index");
            indexBuffer->SetDataNum(static_cast<size_t>(indices.size()));
            indexBuffer->Initialize();
            indexBuffer->SetComponentData("Index", indices);

            SubMeshes[EProceduralGeometry::Cube] = new SubMesh{ vertexBuffer, indexBuffer };
        }

        // Init quad (XY plane, facing Z+)
        {
            TArray<TVector4f> positions{
                { -.5f, -.5f, 0.f, 1.f },
                { -.5f,  .5f, 0.f, 1.f },
                {  .5f,  .5f, 0.f, 1.f },
                {  .5f, -.5f, 0.f, 1.f },
            };
            TArray<TVector2f> uvs{
                { 0.f, 0.f },
                { 0.f, 1.f },
                { 1.f, 1.f },
                { 1.f, 0.f },
            };

            TReflectiveContainerRef vertexBuffer = new ReflectiveContainer();
            vertexBuffer->AddComponent<TVector4f>("Position");
            vertexBuffer->AddComponent<TVector2f>("UV0");
            vertexBuffer->SetDataNum(static_cast<size_t>(positions.size()));
            vertexBuffer->Initialize();
            vertexBuffer->SetComponentData("Position", positions);
            vertexBuffer->SetComponentData("UV0", uvs);

            TArray<uint32> indices{ 0, 1, 2, 0, 2, 3 };
            TReflectiveContainerRef indexBuffer = new ReflectiveContainer();
            indexBuffer->AddComponent<uint32>("Index");
            indexBuffer->SetDataNum(static_cast<size_t>(indices.size()));
            indexBuffer->Initialize();
            indexBuffer->SetComponentData("Index", indices);

            SubMeshes[EProceduralGeometry::Quad] = new SubMesh{ vertexBuffer, indexBuffer };
        }

        // Init triangle (XY plane, facing Z+)
        {
            TArray<TVector4f> positions{
                {  0.f,  .5f, 0.f, 1.f },
                {  .5f, -.5f, 0.f, 1.f },
                { -.5f, -.5f, 0.f, 1.f },
            };
            TArray<TVector2f> uvs{
                { .5f, 1.f },
                { 1.f, 0.f },
                { 0.f, 0.f },
            };

            TReflectiveContainerRef vertexBuffer = new ReflectiveContainer();
            vertexBuffer->AddComponent<TVector4f>("Position");
            vertexBuffer->AddComponent<TVector2f>("UV0");
            vertexBuffer->SetDataNum(static_cast<size_t>(positions.size()));
            vertexBuffer->Initialize();
            vertexBuffer->SetComponentData("Position", positions);
            vertexBuffer->SetComponentData("UV0", uvs);

            TArray<uint32> indices{ 0, 1, 2 };
            TReflectiveContainerRef indexBuffer = new ReflectiveContainer();
            indexBuffer->AddComponent<uint32>("Index");
            indexBuffer->SetDataNum(static_cast<size_t>(indices.size()));
            indexBuffer->Initialize();
            indexBuffer->SetComponentData("Index", indices);

            SubMeshes[EProceduralGeometry::Triangle] = new SubMesh{ vertexBuffer, indexBuffer };
        }
    }

    SubMesh* ProceduralGeometryManager::GetGeometry(EProceduralGeometry geometry)
    {
        auto subMesh = SubMeshes.find(geometry);
        if (subMesh == SubMeshes.end())
        {
            return nullptr;
        }
        return subMesh->second;
    }
}
