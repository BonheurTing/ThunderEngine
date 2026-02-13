#pragma once

#include "RHIResource.h"

namespace Thunder
{
    class RHIUniformBuffer
    {
    public:
        RHIUniformBuffer() = default;
        ~RHIUniformBuffer() = default;

        // Create a new constant buffer and its CBV.
        RHI_API bool Create(uint32 bufferSize);

        // Deferred-delete the old buffer, then create a new one.
        RHI_API bool Update(uint32 bufferSize);

        _NODISCARD_ RHIConstantBuffer* GetConstantBuffer() const { return ConstantBuffer.Get(); }
        _NODISCARD_ RHIConstantBufferView* GetCBV() const { return ConstantBuffer ? ConstantBuffer->GetCBV().Get() : nullptr; }

    private:
        RHIConstantBufferRef ConstantBuffer;
    };
}
