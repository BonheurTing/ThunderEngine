#pragma once
#include <intsafe.h>
#include <stdint.h>

namespace Thunder
{
    class TRHIResource;
    enum D3D12_RESOURCE_STATES;
    #define EResourceStates D3D12_RESOURCE_STATES
    class ID3D12RootSignature;
    #define TRHIRootSignature ID3D12RootSignature
    struct D3D12_CPU_DESCRIPTOR_HANDLE;
    #define TCPUDescriptorHandle D3D12_CPU_DESCRIPTOR_HANDLE
    enum D3D12_CLEAR_FLAGS;
    #define EClearFlags D3D12_CLEAR_FLAGS
    struct tagRECT;
    #define TRect tagRECT
    struct D3D12_INDEX_BUFFER_VIEW;
    #define TIndexBufferView D3D12_INDEX_BUFFER_VIEW
    struct D3D12_VERTEX_BUFFER_VIEW;
    #define TVertexBufferView D3D12_VERTEX_BUFFER_VIEW
    enum D3D_PRIMITIVE_TOPOLOGY;
    #define TPrimitiveTopology D3D_PRIMITIVE_TOPOLOGY
    
    
    class TRHICommandList
    {
    public:
        virtual void* GetCommandList() = 0;
        
        virtual void SetRootSignature(TRHIRootSignature * RootSignature) = 0;
        virtual void SetPrimitiveTopology(TPrimitiveTopology PrimitiveTopology) = 0;
        virtual void SetViewport(float TopLeftX, float TopLeftY, float Width, float Height) = 0;
        virtual void SetScissorRect(UINT NumViewports, long Left, long Top, long Right, long Bottom) = 0;
    
        virtual void SetIndexBuffer( 
            const TIndexBufferView *View) = 0;
            
        virtual void SetVertexBuffers( 
            UINT StartSlot,
            UINT NumViews,
            const TVertexBufferView *Views) = 0;
        
    
        virtual void SetRenderTargets( 
            UINT NumRenderTargetDescriptors,
            const TCPUDescriptorHandle *RenderTargetDescriptors,
            bool RTsSingleHandleToDescriptorRange,
            const TCPUDescriptorHandle *DepthStencilDescriptor) = 0;
               
        virtual void ClearDepthStencilView( 
            TCPUDescriptorHandle DepthStencilView,
            EClearFlags ClearFlags,
            float Depth,
            UINT8 Stencil,
            UINT NumRects,
            const TRect *Rects) = 0;
    
        virtual void ClearRenderTargetView( 
            TCPUDescriptorHandle RenderTargetView,
            const float ColorRGBA[ 4 ],
            UINT NumRects,
            const TRect *Rects) = 0;
        
        virtual void Draw( 
            uint32_t VertexCountPerInstance,
            uint32_t InstanceCount,
            uint32_t StartVertexLocation,
            uint32_t StartInstanceLocation) = 0;
    
        virtual void Transition(
            UINT NumBarriers,
            TRHIResource* Resource,
            EResourceStates stateBefore,
            EResourceStates stateAfter) = 0;
    
    
        virtual void Dispatch( 
            UINT ThreadGroupCountX,
            UINT ThreadGroupCountY,
            UINT ThreadGroupCountZ) = 0;
    
        virtual int Close() = 0;
    };
}
