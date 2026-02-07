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
        TMap<uint16, ShaderResourceParameterInfo> const& GetSamplersIndexMap() const { return SamplersByIndex; }
        TMap<uint16, ShaderResourceParameterInfo> const& GetSRVsIndexMap() const { return SRVsByIndex; }
        TMap<uint16, ShaderResourceParameterInfo> const& GetUAVsIndexMap() const { return UAVsByIndex; }
        TMap<NameHandle, ShaderResourceParameterInfo> const& GetUniformBuffersNameMap() const { return UniformBuffersByName; }
        TMap<NameHandle, ShaderResourceParameterInfo> const& GetSamplersNameMap() const { return SamplersByName; }
        TMap<NameHandle, ShaderResourceParameterInfo> const& GetSRVsNameMap() const { return SRVsByName; }
        TMap<NameHandle, ShaderResourceParameterInfo> const& GetUAVsNameMap() const { return UAVsByName; }

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

    enum class EUniformBufferMemberType : uint8
    {
        Int = 0,
        Float,
        Float4,
        Num
    };
    
    struct UniformBufferMemberEntry
    {
        uint32 Offset = 0xFFFFFFFF;
        uint32 Size = 0xFFFFFFFF;
        EUniformBufferMemberType Type = EUniformBufferMemberType::Num;
    };
    
    class UniformBufferLayout : public RefCountedObject
    {
    public:
        UniformBufferLayout(class ShaderArchive* inShader);
        ~UniformBufferLayout() override = default;

        _NODISCARD_ class ShaderArchive* GetShader() const { return Shader; }
        _NODISCARD_ TMap<NameHandle, UniformBufferMemberEntry> const& GetMemberMap() const { return MemberMap; }
        _NODISCARD_ uint32 GetTotalSize() const { return TotalSize; }

        bool GetMemberEntry(NameHandle name, UniformBufferMemberEntry& outEntry) const
        {
            auto it = MemberMap.find(name);
            if (it != MemberMap.end())
            {
                outEntry = it->second;
                return true;
            }
            return false;
        }

        void AddMember(NameHandle name, UniformBufferMemberEntry const& entry)
        {
            MemberMap[name] = entry;
            TotalSize = std::max(TotalSize, entry.Offset + entry.Size);
        }

        void SetTotalSize(uint32 size)
        {
            TotalSize = size;
        }

    protected:
        // Parent.
        ShaderArchive* Shader = nullptr;
        TMap<NameHandle, UniformBufferMemberEntry> MemberMap;
        uint32 TotalSize = 0;
    };
    using UniformBufferLayoutRef = TRefCountPtr<UniformBufferLayout>;
}
