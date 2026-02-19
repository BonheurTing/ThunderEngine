#pragma optimize("", off)
#include "ShaderBindings.h"

#include "ShaderArchive.h"
#include "ShaderBindingsLayout.h"

namespace Thunder
{
    ShaderBindingsLayout::ShaderBindingsLayout(class ShaderArchive* inShader)
        : Shader(inShader)
    {
    }

    SingleShaderBindings::SingleShaderBindings(uint8* inData)
        : Data(inData)
    {
    }

    SingleShaderBindings::~SingleShaderBindings()
    {
        // Data releasing is handled by ShaderBindings
    }

    void SingleShaderBindings::ClearData()
    {
        // Make sure that "Data" is allocated by TMemory.
        if (Data != nullptr)
        {
            TMemory::Free(Data);
            Data = nullptr;
        }
    }

    void SingleShaderBindings::SetUniformBuffer(const ShaderBindingsLayout* layout, uint32 index, const ShaderBindingHandle& handle) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::UniformBuffer);
        memcpy(Data + offset, &handle, sizeof(ShaderBindingHandle));
    }

    void SingleShaderBindings::SetSampler(const ShaderBindingsLayout* layout, uint32 index, const ShaderBindingHandle& handle) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::Sampler);
        memcpy(Data + offset, &handle, sizeof(ShaderBindingHandle));
    }

    void SingleShaderBindings::SetSRV(const ShaderBindingsLayout* layout, uint32 index, const ShaderBindingHandle& handle) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::SRV);
        memcpy(Data + offset, &handle, sizeof(ShaderBindingHandle));
    }

    void SingleShaderBindings::SetUAV(const ShaderBindingsLayout* layout, uint32 index, const ShaderBindingHandle& handle) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::UAV);
        memcpy(Data + offset, &handle, sizeof(ShaderBindingHandle));
    }

    ShaderBindingHandle SingleShaderBindings::GetUniformBuffer(const ShaderBindingsLayout* layout, uint32 index) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::UniformBuffer);
        ShaderBindingHandle handle;
        memcpy(&handle, Data + offset, sizeof(ShaderBindingHandle));
        return handle;
    }

    ShaderBindingHandle SingleShaderBindings::GetSampler(const ShaderBindingsLayout* layout, uint32 index) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::Sampler);
        ShaderBindingHandle handle;
        memcpy(&handle, Data + offset, sizeof(ShaderBindingHandle));
        return handle;
    }

    ShaderBindingHandle SingleShaderBindings::GetSRV(const ShaderBindingsLayout* layout, uint32 index) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::SRV);
        ShaderBindingHandle handle;
        memcpy(&handle, Data + offset, sizeof(ShaderBindingHandle));
        return handle;
    }

    ShaderBindingHandle SingleShaderBindings::GetUAV(const ShaderBindingsLayout* layout, uint32 index) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::UAV);
        ShaderBindingHandle handle;
        memcpy(&handle, Data + offset, sizeof(ShaderBindingHandle));
        return handle;
    }

    void SingleShaderBindings::SetUniformBuffer(const ShaderBindingsLayout* layout, NameHandle name, const ShaderBindingHandle& handle) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetUniformBuffersNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "UniformBuffer \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return;
        }

        SetUniformBuffer(layout, it->second.Index, handle);
    }

    void SingleShaderBindings::SetSampler(const ShaderBindingsLayout* layout, NameHandle name, const ShaderBindingHandle& handle) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetSamplersNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "Sampler \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return;
        }

        SetSampler(layout, it->second.Index, handle);
    }

    void SingleShaderBindings::SetSRV(const ShaderBindingsLayout* layout, NameHandle name, const ShaderBindingHandle& handle) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetSRVsNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "SRV \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return;
        }

        SetSRV(layout, it->second.Index, handle);
    }

    void SingleShaderBindings::SetUAV(const ShaderBindingsLayout* layout, NameHandle name, const ShaderBindingHandle& handle) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetUAVsNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "UAV \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return;
        }

        SetUAV(layout, it->second.Index, handle);
    }

    ShaderBindingHandle SingleShaderBindings::GetUniformBuffer(const ShaderBindingsLayout* layout, NameHandle name) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetUniformBuffersNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "UniformBuffer \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return ShaderBindingHandle{};
        }

        return GetUniformBuffer(layout, it->second.Index);
    }

    ShaderBindingHandle SingleShaderBindings::GetSampler(const ShaderBindingsLayout* layout, NameHandle name) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetSamplersNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "Sampler \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return ShaderBindingHandle{};
        }

        return GetSampler(layout, it->second.Index);
    }

    ShaderBindingHandle SingleShaderBindings::GetSRV(const ShaderBindingsLayout* layout, NameHandle name) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetSRVsNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "SRV \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return ShaderBindingHandle{};
        }

        return GetSRV(layout, it->second.Index);
    }

    ShaderBindingHandle SingleShaderBindings::GetUAV(const ShaderBindingsLayout* layout, NameHandle name) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetUAVsNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "UAV \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return ShaderBindingHandle{};
        }

        return GetUAV(layout, it->second.Index);
    }

    size_t SingleShaderBindings::CalculateOffset(const ShaderBindingsLayout* layout, uint32 index, EShaderParameterType type)
    {
        // Layout order: UniformBuffer(16 bytes each) → SRV(8 bytes) → UAV(8 bytes) → Sampler(8 bytes).
        size_t viewIndex = 0;
        if (type == EShaderParameterType::UniformBuffer)
        {
            viewIndex = index;
        }
        else if (type == EShaderParameterType::SRV)
        {
            viewIndex = layout->GetUniformBuffersIndexMap().size()
                + index;
        }
        else if (type == EShaderParameterType::UAV)
        {
            viewIndex = layout->GetUniformBuffersIndexMap().size()
               + layout->GetSRVsIndexMap().size()
               + index;
        }
        else if (type == EShaderParameterType::Sampler)
        {
            viewIndex = layout->GetUniformBuffersIndexMap().size()
               + layout->GetSRVsIndexMap().size()
               + layout->GetUAVsIndexMap().size()
               + index;
        }
        size_t offset = viewIndex * sizeof(ShaderBindingHandle);
        return offset;
    }

    ShaderBindings::~ShaderBindings()
    {
        if (!IsTransientAllocated)
        {
            for (uint32 stageIndex = 0; stageIndex < static_cast<uint32>(EShaderStageType::Num); ++stageIndex)
            {
                StageBindings[stageIndex].ClearData();
            }
            Bindings.ClearData();
        }
    }
}
