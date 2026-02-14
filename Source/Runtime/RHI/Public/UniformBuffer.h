#pragma once

#include "RHIResource.h"

namespace Thunder
{

    class RHIUniformBuffer : public RHIResource
    {
    public:
        RHIUniformBuffer(EUniformBufferFlags usage) : RHIResource(RHIResourceDescriptor()), UniformBufferUsage(usage) {}
        ~RHIUniformBuffer() override {}

        // Create a new constant buffer and its CBV.
        RHI_API bool Create(uint32 bufferSize);

        // Deferred-delete the old buffer, then create a new one.
        RHI_API bool UpdateUB(uint32 bufferSize);

        RHI_API bool TestUpdateMultiFrame(uint32 bufferSize);

        _NODISCARD_ RHI_API void* GetResource() const override;
        _NODISCARD_ RHIConstantBufferView* GetCBV() const { return ConstantBuffer ? ConstantBuffer->GetCBV().Get() : nullptr; }

    protected:
        RHIConstantBufferRef ConstantBuffer;
        //RHIUniformBufferRef UniformBuffer;
    public:
        EUniformBufferFlags UniformBufferUsage;
    };

    using RHIUniformBufferRef = TRefCountPtr<class RHIUniformBuffer>;
}
