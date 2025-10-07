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
}
