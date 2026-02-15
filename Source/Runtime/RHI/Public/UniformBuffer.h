#pragma once

#include "RHIResource.h"

namespace Thunder
{

    class RHIUniformBuffer : public RHIResource
    {
    public:
        RHIUniformBuffer(EUniformBufferFlags usage) : RHIResource(RHIResourceDescriptor()), UniformBufferUsage(usage) {}
        ~RHIUniformBuffer() override {}

        RHIUniformBuffer& operator=(const RHIUniformBuffer& rhs)
        {
            UniformBufferUsage = rhs.UniformBufferUsage;
            return *this;
        }

        virtual RHI_API uint64 GetGpuVirtualAddress() = 0;

    public:
        EUniformBufferFlags UniformBufferUsage;
    };

    using RHIUniformBufferRef = TRefCountPtr<class RHIUniformBuffer>;
}
