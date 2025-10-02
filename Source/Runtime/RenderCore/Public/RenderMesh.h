#pragma once

#include "MathUtilities.h"
#include "RenderResource.h"
#include "Container/ReflectiveContainer.h"

namespace Thunder
{
    struct SubMesh : public RefCountedObject
    {
        TReflectiveContainerRef Vertices { nullptr };
        TReflectiveContainerRef Indices { nullptr };
        AABB BoundingBox {};
    };
    using TSubMeshRef = TRefCountPtr<SubMesh>;

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
        RenderStaticMesh(const TArray<TSubMeshRef>& subMeshes);

    private:
        void CreateMesh_RenderThread() override;
    };
}
