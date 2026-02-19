
#include "ShaderParameterMap.h"

namespace Thunder
{
	void ShaderParameterMap::Reset()
	{
		IntParameters.clear();
		FloatParameters.clear();
		VectorParameters.clear();
		TextureParameters.clear();
		StaticSwitchParameters.clear();
	}

	void ShaderParameterMap::SetIntParameter(const NameHandle& paramName, int32 value)
	{
		IntParameters[paramName] = value;
	}

	void ShaderParameterMap::SetFloatParameter(const NameHandle& paramName, float value)
	{
		FloatParameters[paramName] = value;
	}

	void ShaderParameterMap::SetVectorParameter(const NameHandle& paramName, const TVector4f& value)
	{
		VectorParameters[paramName] = value;
	}

	void ShaderParameterMap::SetTextureParameter(const NameHandle& paramName, const TGuid& textureGuid)
	{
		TextureParameters[paramName] = textureGuid;
	}

	void ShaderParameterMap::SetTextureParameter(const NameHandle& paramName, uint32 textureId)
	{
		TextureParameters[paramName] = TGuid(textureId);
	}

	void ShaderParameterMap::SetStaticSwitchParameter(const NameHandle& paramName, bool value)
	{
		StaticSwitchParameters[paramName] = value;
	}

	bool ShaderParameterMap::GetIntParameter(const NameHandle& paramName, int32& outValue) const
	{
		auto it = IntParameters.find(paramName);
		if (it != IntParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool ShaderParameterMap::GetFloatParameter(const NameHandle& paramName, float& outValue) const
	{
		auto it = FloatParameters.find(paramName);
		if (it != FloatParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool ShaderParameterMap::GetVectorParameter(const NameHandle& paramName, TVector4f& outValue) const
	{
		auto it = VectorParameters.find(paramName);
		if (it != VectorParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool ShaderParameterMap::GetTextureParameter(const NameHandle& paramName, TGuid& outTextureGuid) const
	{
		auto it = TextureParameters.find(paramName);
		if (it != TextureParameters.end())
		{
			outTextureGuid = it->second;
			return true;
		}
		return false;
	}

	bool ShaderParameterMap::GetStaticSwitchParameter(const NameHandle& paramName, bool& outValue) const
	{
		auto it = StaticSwitchParameters.find(paramName);
		if (it != StaticSwitchParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	void ShaderParameterMap::RemoveIntParameter(const NameHandle& paramName)
	{
		IntParameters.erase(paramName);
	}

	void ShaderParameterMap::RemoveFloatParameter(const NameHandle& paramName)
	{
		FloatParameters.erase(paramName);
	}

	void ShaderParameterMap::RemoveVectorParameter(const NameHandle& paramName)
	{
		VectorParameters.erase(paramName);
	}

	void ShaderParameterMap::RemoveTextureParameter(const NameHandle& paramName)
	{
		TextureParameters.erase(paramName);
	}

	void ShaderParameterMap::RemoveStaticSwitchParameter(const NameHandle& paramName)
	{
		StaticSwitchParameters.erase(paramName);
	}
}
