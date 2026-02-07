#include "ShaderBindingsLayout.h"

#include "ShaderArchive.h"

namespace Thunder
{
    ShaderBindingsLayout::ShaderBindingsLayout(class ShaderArchive* inShader)
        : Shader(inShader)
    {
    }

    void ShaderBindingsLayout::AddBinding(ShaderResourceParameterInfo const& bindingInfo)
    {
        if (bindingInfo.Type == EShaderParameterType::UniformBuffer)
        {
            UniformBuffersByIndex[bindingInfo.Index] = bindingInfo;
            UniformBuffersByName[bindingInfo.Name] = bindingInfo;
        }
        else if (bindingInfo.Type == EShaderParameterType::Sampler)
        {
            SamplersByIndex[bindingInfo.Index] = bindingInfo;
            SamplersByName[bindingInfo.Name] = bindingInfo;
        }
        else if (bindingInfo.Type == EShaderParameterType::SRV)
        {
            SRVsByIndex[bindingInfo.Index] = bindingInfo;
            SRVsByName[bindingInfo.Name] = bindingInfo;
        }
        else if (bindingInfo.Type == EShaderParameterType::UAV)
        {
            UAVsByIndex[bindingInfo.Index] = bindingInfo;
            UAVsByName[bindingInfo.Name] = bindingInfo;
        }
        else
        {
            TAssertf(false, "Unknown binding type %u.", static_cast<uint32>(bindingInfo.Type));
        }
    }

    size_t ShaderBindingsLayout::GetTotalSize() const
    {
        return (UniformBuffersByName.size() * sizeof(UniformBufferBindingHandle))
            + (SRVsByName.size() * sizeof(ShaderBindingHandle))
            + (UAVsByName.size() * sizeof(ShaderBindingHandle))
            + (SamplersByName.size() * sizeof(ShaderBindingHandle));
    }

    UniformBufferLayout::UniformBufferLayout(class ShaderArchive* inShader)
        : Shader(inShader), TotalSize(0)
    {
    }
}
