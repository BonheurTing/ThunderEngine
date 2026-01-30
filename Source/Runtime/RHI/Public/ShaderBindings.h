#pragma once

#include "IDynamicRHI.h"
#include "IRHIModule.h"
#include "RHIContext.h"
#include "RHIResource.h"
#include "ShaderBindingsLayout.h"
#include "ShaderDefinition.h"

namespace Thunder
{
    class ShaderBindingsLayout;

    class SingleShaderBindings
    {
    public:
        SingleShaderBindings(byte* inData = nullptr);
        ~SingleShaderBindings();

        void SetData(byte* inData) { Data = inData; }
        byte* GetData() const { return Data; }
        void ClearData();

        // By index.
        void SetUniformBuffer(const ShaderBindingsLayout* layout, uint32 index, const UniformBufferBindingHandle& handle) const;
        void SetSampler(const ShaderBindingsLayout* layout, uint32 index, const ShaderBindingHandle& handle) const;
        void SetSRV(const ShaderBindingsLayout* layout, uint32 index, const ShaderBindingHandle& handle) const;
        void SetUAV(const ShaderBindingsLayout* layout, uint32 index, const ShaderBindingHandle& handle) const;
        UniformBufferBindingHandle GetUniformBuffer(const ShaderBindingsLayout* layout, uint32 index) const;
        ShaderBindingHandle GetSampler(const ShaderBindingsLayout* layout, uint32 index) const;
        ShaderBindingHandle GetSRV(const ShaderBindingsLayout* layout, uint32 index) const;
        ShaderBindingHandle GetUAV(const ShaderBindingsLayout* layout, uint32 index) const;

        // By name.
        void SetUniformBuffer(const ShaderBindingsLayout* layout, NameHandle name, const UniformBufferBindingHandle& handle) const;
        void SetSampler(const ShaderBindingsLayout* layout, NameHandle name, const ShaderBindingHandle& handle) const;
        void SetSRV(const ShaderBindingsLayout* layout, NameHandle name, const ShaderBindingHandle& handle) const;
        void SetUAV(const ShaderBindingsLayout* layout, NameHandle name, const ShaderBindingHandle& handle) const;
        UniformBufferBindingHandle GetUniformBuffer(const ShaderBindingsLayout* layout, NameHandle name) const;
        ShaderBindingHandle GetSampler(const ShaderBindingsLayout* layout, NameHandle name) const;
        ShaderBindingHandle GetSRV(const ShaderBindingsLayout* layout, NameHandle name) const;
        ShaderBindingHandle GetUAV(const ShaderBindingsLayout* layout, NameHandle name) const;

        static size_t CalculateOffset(const ShaderBindingsLayout* layout, uint32 index, EShaderParameterType type);

    private:
        byte* Data = nullptr;
    };
    static_assert(sizeof(SingleShaderBindings) == 8);

    class ShaderBindings
    {
    public:
        ShaderBindings() = default;
        ~ShaderBindings();

        void SetTransientAllocated(bool isTransientAllocated) { IsTransientAllocated = isTransientAllocated; }

        SingleShaderBindings* GetSingleShaderBindings() { return &(Bindings); }
        void SetBindingsData(byte* inData) { Bindings.SetData(inData); }

        SingleShaderBindings* GetStageSingleShaderBindings(EShaderStageType stage) { return &(StageBindings[static_cast<uint32>(stage)]); }
        void SetStageBindingsData(EShaderStageType stage, byte* inData) { StageBindings[static_cast<uint32>(stage)].SetData(inData); }

    private:
        SingleShaderBindings StageBindings[static_cast<uint8>(EShaderStageType::Num)];
        SingleShaderBindings Bindings;
        bool IsTransientAllocated = false;
    };
    static_assert(sizeof(ShaderBindings) == 80);
}
