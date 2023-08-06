#pragma once
#include <cstdint>
#include <intsafe.h>
class TRHICommandList;

struct D3D12_COMMAND_QUEUE_DESC;
#define TCommandQueueDesc D3D12_COMMAND_QUEUE_DESC
class ID3D12CommandQueue;
#define TCommandQueue ID3D12CommandQueue
class ID3D12Fence;
class IDXGISwapChain3;
class RHITexture;

class TRHIContext
{
public:
    
    virtual void Execute(TRHICommandList *CommandListsPtr) = 0;

    virtual UINT GetFrameIndex() = 0;

    virtual int GetBuffer(UINT Index, RHITexture** OutBuffer) = 0;

    // synchronization objects
    virtual void CreateSynchronizationObjects() = 0;
    
    virtual UINT WaitForPreviousFrame() = 0;
    
    virtual void TempDestroy() = 0;
};
