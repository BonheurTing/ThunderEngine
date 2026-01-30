#pragma once

#include "ShaderDefinition.h"

namespace Thunder
{
    enum class EShaderParameterType : uint8
    {
        UniformBuffer = 0,
        Sampler,
        SRV,
        UAV,
        Num
    };

    class ShaderResourceParameterInfo
    {
    public:
        NameHandle Name{};
        uint16 Index = 0xFFFF;
        EShaderParameterType Type = EShaderParameterType::Num;
    };

    struct UniformBufferBindingHandle
    {
        uint64 Base = 0xFFFFFFFFFFFFFFFF;
        uint64 Offset = 0xFFFFFFFFFFFFFFFF;
    };

    struct ShaderBindingHandle
    {
        uint64 Handle = 0xFFFFFFFFFFFFFFFF;
    };

    class ShaderBindingsLayout : public RefCountedObject
    {
    public:
        ShaderBindingsLayout(class ShaderArchive* inShader);
        ~ShaderBindingsLayout() override = default;

        void AddBinding(ShaderResourceParameterInfo const& bindingInfo);

        class ShaderArchive* GetShader() const { return Shader; }
        size_t GetTotalSize() const;

        TMap<uint16, ShaderResourceParameterInfo> const& GetUniformBuffersIndexMap() const { return UniformBuffersByIndex; }
        TMap<uint16, ShaderResourceParameterInfo> const& GetSamplersIndexMap() const { return SRVsByIndex; }
        TMap<uint16, ShaderResourceParameterInfo> const& GetSRVsIndexMap() const { return UAVsByIndex; }
        TMap<uint16, ShaderResourceParameterInfo> const& GetUAVsIndexMap() const { return SamplersByIndex; }
        TMap<NameHandle, ShaderResourceParameterInfo> const& GetUniformBuffersNameMap() const { return UniformBuffersByName; }
        TMap<NameHandle, ShaderResourceParameterInfo> const& GetSamplersNameMap() const { return SRVsByName; }
        TMap<NameHandle, ShaderResourceParameterInfo> const& GetSRVsNameMap() const { return UAVsByName; }
        TMap<NameHandle, ShaderResourceParameterInfo> const& GetUAVsNameMap() const { return SamplersByName; }

    protected:
        // Parent.
        ShaderArchive* Shader = nullptr;

        // Index maps.
        TMap<uint16, ShaderResourceParameterInfo> UniformBuffersByIndex;
        TMap<uint16, ShaderResourceParameterInfo> SRVsByIndex;
        TMap<uint16, ShaderResourceParameterInfo> UAVsByIndex;
        TMap<uint16, ShaderResourceParameterInfo> SamplersByIndex;

        // Name maps.
        TMap<NameHandle, ShaderResourceParameterInfo> UniformBuffersByName;
        TMap<NameHandle, ShaderResourceParameterInfo> SRVsByName;
        TMap<NameHandle, ShaderResourceParameterInfo> UAVsByName;
        TMap<NameHandle, ShaderResourceParameterInfo> SamplersByName;
    };
    using ShaderBindingsLayoutRef = TRefCountPtr<ShaderBindingsLayout>;
}
