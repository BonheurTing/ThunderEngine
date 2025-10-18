#include "Material.h"
#include "FileSystem/MemoryArchive.h"

namespace Thunder
{
	Material::Material(GameObject* inOuter)
		: IMaterial(inOuter)
	{
	}

	void Material::OnResourceLoaded()
	{
		GameResource::OnResourceLoaded();
		LOG("Material loaded: %s", GetResourceName().c_str());
	}

	void Material::Serialize(MemoryWriter& archive)
	{
		IMaterial::Serialize(archive);

		// Serialize shader archive (stored as pointer, serialized as name/path)
		// Note: In a real implementation, you might serialize the shader archive name
		// and load it separately as a dependency

		// Serialize int parameters
		uint32 intParamCount = static_cast<uint32>(IntParam.size());
		archive << intParamCount;
		for (const auto& pair : IntParam)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}

		// Serialize float parameters
		uint32 floatParamCount = static_cast<uint32>(FloatParam.size());
		archive << floatParamCount;
		for (const auto& pair : FloatParam)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}

		// Serialize color parameters
		uint32 colorParamCount = static_cast<uint32>(ColorParam.size());
		archive << colorParamCount;
		for (const auto& pair : ColorParam)
		{
			archive << pair.first.ToString();
			archive << pair.second.X << pair.second.Y << pair.second.Z << pair.second.W;
		}

		// Serialize texture parameters (as GUIDs)
		uint32 textureParamCount = static_cast<uint32>(TextureParam.size());
		archive << textureParamCount;
		for (const auto& pair : TextureParam)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}
	}

	void Material::DeSerialize(MemoryReader& archive)
	{
		IMaterial::DeSerialize(archive);

		// Deserialize int parameters
		uint32 intParamCount = 0;
		archive >> intParamCount;
		IntParam.clear();
		for (uint32 i = 0; i < intParamCount; ++i)
		{
			String paramName;
			int32 value;
			archive >> paramName;
			archive >> value;
			IntParam[NameHandle(paramName)] = value;
		}

		// Deserialize float parameters
		uint32 floatParamCount = 0;
		archive >> floatParamCount;
		FloatParam.clear();
		for (uint32 i = 0; i < floatParamCount; ++i)
		{
			String paramName;
			float value;
			archive >> paramName;
			archive >> value;
			FloatParam[NameHandle(paramName)] = value;
		}

		// Deserialize color parameters
		uint32 colorParamCount = 0;
		archive >> colorParamCount;
		ColorParam.clear();
		for (uint32 i = 0; i < colorParamCount; ++i)
		{
			String paramName;
			TVector4f value;
			archive >> paramName;
			archive >> value.X >> value.Y >> value.Z >> value.W;
			ColorParam[NameHandle(paramName)] = value;
		}

		// Deserialize texture parameters
		uint32 textureParamCount = 0;
		archive >> textureParamCount;
		TextureParam.clear();
		for (uint32 i = 0; i < textureParamCount; ++i)
		{
			String paramName;
			TGuid value;
			archive >> paramName;
			archive >> value;
			TextureParam[NameHandle(paramName)] = value;
		}
	}

	// Parameter setters
	void Material::SetIntParameter(const NameHandle& paramName, int32 value)
	{
		IntParam[paramName] = value;
	}

	void Material::SetFloatParameter(const NameHandle& paramName, float value)
	{
		FloatParam[paramName] = value;
	}

	void Material::SetColorParameter(const NameHandle& paramName, const TVector4f& value)
	{
		ColorParam[paramName] = value;
	}

	void Material::SetTextureParameter(const NameHandle& paramName, const TGuid& textureGuid)
	{
		TextureParam[paramName] = textureGuid;
		// Add texture GUID to dependencies
		AddDependency(textureGuid);
	}

	// Parameter getters
	bool Material::GetIntParameter(const NameHandle& paramName, int32& outValue) const
	{
		auto it = IntParam.find(paramName);
		if (it != IntParam.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool Material::GetFloatParameter(const NameHandle& paramName, float& outValue) const
	{
		auto it = FloatParam.find(paramName);
		if (it != FloatParam.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool Material::GetColorParameter(const NameHandle& paramName, TVector4f& outValue) const
	{
		auto it = ColorParam.find(paramName);
		if (it != ColorParam.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool Material::GetTextureParameter(const NameHandle& paramName, TGuid& outTextureGuid) const
	{
		auto it = TextureParam.find(paramName);
		if (it != TextureParam.end())
		{
			outTextureGuid = it->second;
			return true;
		}
		return false;
	}

	// Parameter removal
	void Material::RemoveIntParameter(const NameHandle& paramName)
	{
		IntParam.erase(paramName);
	}

	void Material::RemoveFloatParameter(const NameHandle& paramName)
	{
		FloatParam.erase(paramName);
	}

	void Material::RemoveColorParameter(const NameHandle& paramName)
	{
		ColorParam.erase(paramName);
	}

	void Material::RemoveTextureParameter(const NameHandle& paramName)
	{
		TextureParam.erase(paramName);
	}
}
