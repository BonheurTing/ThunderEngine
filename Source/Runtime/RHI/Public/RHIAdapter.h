#pragma once

#include <intsafe.h>
class TVertexBuffer;
class RHITexture;
class TRHICommandList;
class TRHIContext;
struct CD3DX12_CPU_DESCRIPTOR_HANDLE;

class TRHIAdapter
{
public:
    virtual void CreatePipelineStateObject() = 0;
    virtual void CreateCommandAllocator() = 0;
    virtual void CreateCommandList(TRHICommandList** OutRHICommandList) = 0;
    virtual void CreateVertexBuffer(UINT VerticesSize, void* Vertices, const UINT VertexBufferSize, TVertexBuffer** OutVertexBuffer) = 0;
    virtual void CreateRenderTargetView(RHITexture * RenderTarget, int RTOffset) = 0;
    virtual void CreateRootSignature() = 0;
    
    virtual void* GetRootSignature() = 0;
    virtual void* GetPipelineStateObject() = 0;
    virtual CD3DX12_CPU_DESCRIPTOR_HANDLE GetHeapHandle(int Index) = 0;

    virtual void ResetCommandList(TRHICommandList* InCommandList) = 0;

    inline TRHIContext * GetDefaultContext() { return DefaultContext; }
    inline void SetDefaultContext(TRHIContext * InRHIContext) { DefaultContext = InRHIContext; }

private:
    TRHIContext * DefaultContext = nullptr;
};