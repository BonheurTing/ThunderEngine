#pragma once

#include "DXSample.h"

using namespace DirectX;
class TRHIContext;
class TRHICommandList;
class TRHIAdapter;
class TVertexBuffer;
class RHITexture;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12HelloTriangle : public DXSample
{
public:
    D3D12HelloTriangle(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
    static const UINT FrameCount = 2;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    // Pipeline objects.
    TRHIAdapter * RHIAdapter;
    TRHIContext * RHIContext;
    TRHICommandList* RHICommandList;
    RHITexture* RenderTarget[FrameCount];

    // App resources.
    TVertexBuffer* VertexBuffer;

    // Synchronization objects.
    UINT FrameIndex;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
};
