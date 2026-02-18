#pragma once

#include "MathUtilities.h"
#include "RenderResource.h"

namespace Thunder
{
    struct SubMesh
    {
        TReflectiveContainerRef Vertices { nullptr };
        TReflectiveContainerRef Indices { nullptr };
        AABB BoundingBox {};
        uint32 SubMeshIndex{ 0 };

        SubMesh() = delete;
        SubMesh(uint32 index) : SubMeshIndex(index) {}
        SubMesh(const TReflectiveContainerRef& inVertices, const TReflectiveContainerRef& inIndices) : Vertices(inVertices), Indices(inIndices) {}

        bool GetVertexDeclaration(TArray<RHIVertexElement>& outDeclarations) const;
        bool GetVertexDeclaration(RHIVertexDeclarationDescriptor& outDeclarations) const;
    };

    class RenderMesh : public RenderResource
    {
    public:
        TArray<RHIVertexBufferRef> VBs;
        TArray<RHIIndexBufferRef> IBs;

        void InitRHI() override;
        void ReleaseRHI() override;

        bool isDynamic() const override { return bDynamic; }

    protected:
        virtual void CreateMesh_RenderThread() = 0;

        TArray<TReflectiveContainerRef> RawVerticesData;
        TArray<TReflectiveContainerRef> RawIndicesData;
        
        uint32 NumSubMeshes = 0;
        bool bDynamic = false;
    };
    using RenderMeshRef = TRefCountPtr<RenderMesh>;

    class RenderStaticMesh final : public RenderMesh
    {
    public:
        RenderStaticMesh(const TArray<SubMesh*>& subMeshes);

    private:
        void CreateMesh_RenderThread() override;
    };

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
