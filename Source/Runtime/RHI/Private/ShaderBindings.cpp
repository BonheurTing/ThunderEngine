#include "ShaderBindings.h"

#include "ShaderArchive.h"
#include "ShaderBindingsLayout.h"

namespace Thunder
{
    namespace
    {
        // Helper function to calculate offset in Data array
        size_t CalculateOffset(const ShaderBindingsLayout* layout, uint32 index, EShaderParameterType type)
        {
            size_t offset = 0;

            // Layout order: UniformBuffer(16 bytes each) → SRV(8 bytes) → UAV(8 bytes) → Sampler(8 bytes)
            if (type == EShaderParameterType::UniformBuffer)
            {
                offset = index * sizeof(UniformBufferBindingHandle);
            }
            else if (type == EShaderParameterType::SRV)
            {
                offset = layout->GetUniformBuffersIndexMap().size() * sizeof(UniformBufferBindingHandle)
                       + index * sizeof(ShaderBindingHandle);
            }
            else if (type == EShaderParameterType::UAV)
            {
                offset = layout->GetUniformBuffersIndexMap().size() * sizeof(UniformBufferBindingHandle)
                       + layout->GetSRVsIndexMap().size() * sizeof(ShaderBindingHandle)
                       + index * sizeof(ShaderBindingHandle);
            }
            else if (type == EShaderParameterType::Sampler)
            {
                offset = layout->GetUniformBuffersIndexMap().size() * sizeof(UniformBufferBindingHandle)
                       + layout->GetSRVsIndexMap().size() * sizeof(ShaderBindingHandle)
                       + layout->GetUAVsIndexMap().size() * sizeof(ShaderBindingHandle)
                       + index * sizeof(ShaderBindingHandle);
            }

            return offset;
        }
    }

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
        ClearData();
    }

    void SingleShaderBindings::ClearData()
    {
        if (Data != nullptr)
        {
            delete[] Data;
            Data = nullptr;
        }
    }

    void SingleShaderBindings::SetUniformBuffer(const ShaderBindingsLayout* layout, uint32 index, const UniformBufferBindingHandle& handle) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::UniformBuffer);
        memcpy(Data + offset, &handle, sizeof(UniformBufferBindingHandle));
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

    UniformBufferBindingHandle SingleShaderBindings::GetUniformBuffer(const ShaderBindingsLayout* layout, uint32 index) const
    {
        TAssert(Data != nullptr);
        TAssert(layout != nullptr);

        size_t offset = CalculateOffset(layout, index, EShaderParameterType::UniformBuffer);
        UniformBufferBindingHandle handle;
        memcpy(&handle, Data + offset, sizeof(UniformBufferBindingHandle));
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

    void SingleShaderBindings::SetUniformBuffer(const ShaderBindingsLayout* layout, NameHandle name, const UniformBufferBindingHandle& handle) const
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

    UniformBufferBindingHandle SingleShaderBindings::GetUniformBuffer(const ShaderBindingsLayout* layout, NameHandle name) const
    {
        TAssert(layout != nullptr);

        auto const& nameMap = layout->GetUniformBuffersNameMap();
        auto it = nameMap.find(name);
        if (it == nameMap.end()) [[unlikely]]
        {
            TAssertf(false, "UniformBuffer \"%s\" not found in shader archive \"%s\".", name.c_str(), layout->GetShader()->GetName().c_str());
            return UniformBufferBindingHandle{};
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
        size_t offset = 0;

        // Layout order: UniformBuffer(16 bytes each) → SRV(8 bytes) → UAV(8 bytes) → Sampler(8 bytes).
        if (type == EShaderParameterType::UniformBuffer)
        {
            offset = index * sizeof(UniformBufferBindingHandle);
        }
        else if (type == EShaderParameterType::SRV)
        {
            offset = layout->GetUniformBuffersIndexMap().size() * sizeof(UniformBufferBindingHandle)
                   + index * sizeof(ShaderBindingHandle);
        }
        else if (type == EShaderParameterType::UAV)
        {
            offset = layout->GetUniformBuffersIndexMap().size() * sizeof(UniformBufferBindingHandle)
                   + layout->GetSRVsIndexMap().size() * sizeof(ShaderBindingHandle)
                   + index * sizeof(ShaderBindingHandle);
        }
        else if (type == EShaderParameterType::Sampler)
        {
            offset = layout->GetUniformBuffersIndexMap().size() * sizeof(UniformBufferBindingHandle)
                   + layout->GetSRVsIndexMap().size() * sizeof(ShaderBindingHandle)
                   + layout->GetUAVsIndexMap().size() * sizeof(ShaderBindingHandle)
                   + index * sizeof(ShaderBindingHandle);
        }

        return offset;
    }

    ShaderBindings::~ShaderBindings()
    {
        for (uint32 stageIndex = 0; stageIndex < static_cast<uint32>(EShaderStageType::Num); ++stageIndex)
        {
            Bindings[stageIndex].ClearData();
        }
    }
}
