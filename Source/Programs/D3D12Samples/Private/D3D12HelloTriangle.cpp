#include "stdafx.h"
#include "D3D12HelloTriangle.h"
#include "D3D12CommandList.h"
#include "D3D12Context.h"
#include "D3D12Adapter.h"
#include "D3D12Resource.h"

D3D12HelloTriangle::D3D12HelloTriangle(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    FrameIndex(0)
{
}

void D3D12HelloTriangle::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

// Load the rendering pipeline dependencies.
void D3D12HelloTriangle::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif
    
    RHIAdapter = new TD3D12Adapter(FrameCount, m_width, m_height, Win32Application::GetHwnd());
    RHIContext = RHIAdapter->GetDefaultContext();
    FrameIndex = RHIContext->GetFrameIndex();

    // Create frame resources.
    {
        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            RenderTarget[n] = new TD3D12Texture();
            ThrowIfFailed(RHIContext->GetBuffer(n, &RenderTarget[n]));
            RHIAdapter->CreateRenderTargetView(RenderTarget[n], n);
        }
    }

    RHIAdapter->CreateCommandAllocator();
}

// Load the sample assets.
void D3D12HelloTriangle::LoadAssets()
{
    // Create an empty root signature.
    RHIAdapter->CreateRootSignature();

    // Create the pipeline state, which includes compiling and loading shaders.
    RHIAdapter->CreatePipelineStateObject();
    
    // Create the command list.
    RHIAdapter->CreateCommandList(&RHICommandList);
    
    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(RHICommandList->Close());
    
    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT VertexBufferSize = sizeof(triangleVertices);

        VertexBuffer = new TD3D12VertexBuffer();
        RHIAdapter->CreateVertexBuffer(VertexBufferSize, triangleVertices, VertexBufferSize, &VertexBuffer);
        VertexBuffer->InitializeVertexBufferView(sizeof(Vertex), VertexBufferSize);
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        RHIContext->CreateSynchronizationObjects();
        FrameIndex = RHIContext->WaitForPreviousFrame();
    }
}

// Update frame-based values.
void D3D12HelloTriangle::OnUpdate()
{
}

// Render the scene.
void D3D12HelloTriangle::OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    RHIContext->Execute(RHICommandList);
    
    FrameIndex = RHIContext->WaitForPreviousFrame();
}

void D3D12HelloTriangle::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    FrameIndex = RHIContext->WaitForPreviousFrame();

    RHIContext->TempDestroy();
}

void D3D12HelloTriangle::PopulateCommandList()
{
    RHIAdapter->ResetCommandList(RHICommandList);

    // Set necessary state.
    ID3D12RootSignature* RootSignature = static_cast<ID3D12RootSignature*>(RHIAdapter->GetRootSignature());
    ThrowIfFailed(RootSignature != nullptr);
    RHICommandList->SetRootSignature(RootSignature);
    RHICommandList->SetViewport(0, 0, static_cast<float>(m_width), static_cast<float>(m_height));
    RHICommandList->SetScissorRect(1, 0, 0, static_cast<long>(m_width), static_cast<long>(m_height));

    // Indicate that the back buffer will be used as a render target.
    RHICommandList->Transition(1, RenderTarget[FrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = RHIAdapter->GetHeapHandle(FrameIndex);
    RHICommandList->SetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    RHICommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    RHICommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    void* TempVertexBufferView = VertexBuffer->GetVertexBufferView();
    ThrowIfFailed(TempVertexBufferView != nullptr);
    TVertexBufferView * Vbv = static_cast<D3D12_VERTEX_BUFFER_VIEW*>(TempVertexBufferView);
    RHICommandList->SetVertexBuffers(0, 1, Vbv);
    RHICommandList->Draw(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    RHICommandList->Transition(1, RenderTarget[FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    ThrowIfFailed(RHICommandList->Close());
}
