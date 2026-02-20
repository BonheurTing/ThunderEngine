#pragma once

#include "MathUtilities.h"
#include "RenderResource.h"

namespace Thunder
{
    class SubMesh
    {
    public:
        SubMesh() = delete;
        SubMesh(uint32 index) : SubMeshIndex(index) {}
        SubMesh(TReflectiveContainerRef inVertices, TReflectiveContainerRef inIndices)
            : Vertices(std::move(inVertices)), Indices(std::move(inIndices)) {}

        TReflectiveContainerRef GetVertices() const { return Vertices; }
        TReflectiveContainerRef GetIndices() const { return Indices; }
        AABB& GetBoundingBox() { return BoundingBox; }
        uint32 GetSubMeshIndex() const { return SubMeshIndex; }
        void SetContents(TReflectiveContainerRef inVertices, TReflectiveContainerRef inIndices, AABB bb)
        {
            Vertices = std::move(inVertices);
            Indices = std::move(inIndices);
            BoundingBox = std::move(bb);
        }

        bool GetVertexDeclaration(TArray<RHIVertexElement>& outDeclarations) const;
        bool GetVertexDeclaration(RHIVertexDeclarationDescriptor& outDeclarations) const;

        void InitRHI();
        void ReleaseRHI();
        RHIVertexBufferRef GetVerticesBuffer() const { return VerticesBuffer; }
        RHIIndexBufferRef GetIndicesBuffer() const { return IndicesBuffer; }
        
    private:
        // Game
        TReflectiveContainerRef Vertices { nullptr };
        TReflectiveContainerRef Indices { nullptr };
        AABB BoundingBox {};
        uint32 SubMeshIndex{ 0 };
        // Render
        RHIVertexBufferRef VerticesBuffer { nullptr };
        RHIIndexBufferRef IndicesBuffer { nullptr };
    };

    class RenderMesh : public RenderResource
    {
    public:
        RenderMesh(const TArray<SubMesh*>& subMeshes);
        ~RenderMesh() override;

        void InitRHI() override;
        void ReleaseRHI() override;

        bool isDynamic() const override { return bDynamic; }

    private:
        void CreateMesh_RenderThread();

        TArray<SubMesh*> SubMeshes; // Manage its lifetime
        bool bDynamic = false;
    };
    using RenderMeshRef = TRefCountPtr<RenderMesh>;

    enum class RENDERCORE_API EProceduralGeometry : uint32
    {
        Cube,
        Quad,
        Triangle,
        Num
    };

    class ProceduralGeometryManager
    {
    public:
        ProceduralGeometryManager() = default;
        ~ProceduralGeometryManager();
        void InitBasicGeometrySubMeshes();

        RENDERCORE_API SubMesh* GetGeometry(EProceduralGeometry geometry);
    private:
        TMap<EProceduralGeometry, SubMesh*> SubMeshes;
    };

    extern RENDERCORE_API ProceduralGeometryManager* GProceduralGeometryManager;
}
