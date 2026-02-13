#include "UniformBuffer.h"
#include "IDynamicRHI.h"

namespace Thunder
{
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

    bool RHIUniformBuffer::Update(uint32 bufferSize)
    {
        // Defer-delete the old buffer so the GPU can finish using it.
        if (ConstantBuffer.IsValid())
        {
            RHIDeferredDeleteResource(std::move(ConstantBuffer));
        }

        return Create(bufferSize);
    }
}
