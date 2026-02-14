#pragma optimize("", off)
#include "UniformBuffer.h"
#include "IDynamicRHI.h"

namespace Thunder
{
    void* RHIUniformBuffer::GetResource() const
    {
        if (ConstantBuffer.IsValid())
        {
            return ConstantBuffer.Get()->GetResource();
        }
        return nullptr;
    }

    bool RHIUniformBuffer::Create(uint32 bufferSize)
    {
        ConstantBuffer = RHICreateConstantBuffer(bufferSize, EBufferCreateFlags::Dynamic);
        if (!ConstantBuffer) [[unlikely]]
        {
            TAssertf(false, "Failed to create constant buffer (size: %u).", bufferSize);
            return false;
        }
        RHICreateConstantBufferView(*(ConstantBuffer.Get()), bufferSize);
        return true;
    }

    bool RHIUniformBuffer::UpdateUB(uint32 bufferSize)
    {
        // Defer-delete the old buffer so the GPU can finish using it.
        if (ConstantBuffer.IsValid())
        {
            RHIDeferredDeleteResource(std::move(ConstantBuffer));
        }

        return Create(bufferSize);
    }

    bool RHIUniformBuffer::TestUpdateMultiFrame(uint32 bufferSize)
    {
        //if (UniformBuffer.IsValid())
        {
            //RHIUpdateUniformBuffer();
        }

        RHICreateUniformBuffer(bufferSize, EUniformBufferFlags::UniformBuffer_MultiFrame, nullptr);
        return true;
    }
}
#pragma optimize("", on)