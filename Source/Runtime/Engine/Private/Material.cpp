#include "Material.h"
#include "FileSystem/MemoryArchive.h"

namespace Thunder
{
	Material::Material(GameObject* InOuter)
		: IMaterial(InOuter)
	{
	}

	void Material::OnResourceLoaded()
	{
		GameResource::OnResourceLoaded();
		LOG("Material loaded: %s", GetResourceName().c_str());
	}

	void Material::Serialize(MemoryWriter& Archive)
	{
		IMaterial::Serialize(Archive);

		// Serialize shader archive (stored as pointer, serialized as name/path)
		// Note: In a real implementation, you might serialize the shader archive name
		// and load it separately as a dependency

		// Serialize int parameters
		uint32 IntParamCount = static_cast<uint32>(IntParam.size());
		Archive << IntParamCount;
		for (const auto& Pair : IntParam)
		{
			Archive << Pair.first.ToString();
			Archive << Pair.second;
		}

		// Serialize float parameters
		uint32 FloatParamCount = static_cast<uint32>(FloatParam.size());
		Archive << FloatParamCount;
		for (const auto& Pair : FloatParam)
		{
			Archive << Pair.first.ToString();
			Archive << Pair.second;
		}

		// Serialize color parameters
		uint32 ColorParamCount = static_cast<uint32>(ColorParam.size());
		Archive << ColorParamCount;
		for (const auto& Pair : ColorParam)
		{
			Archive << Pair.first.ToString();
			Archive << Pair.second.X << Pair.second.Y << Pair.second.Z << Pair.second.W;
		}

		// Serialize texture parameters (as GUIDs)
		uint32 TextureParamCount = static_cast<uint32>(TextureParam.size());
		Archive << TextureParamCount;
		for (const auto& Pair : TextureParam)
		{
			Archive << Pair.first.ToString();
			Archive << Pair.second;
		}
	}

	void Material::DeSerialize(MemoryReader& Archive)
	{
		IMaterial::DeSerialize(Archive);

		// Deserialize int parameters
		uint32 IntParamCount = 0;
		Archive >> IntParamCount;
		IntParam.clear();
		for (uint32 i = 0; i < IntParamCount; ++i)
		{
			String ParamName;
			int32 Value;
			Archive >> ParamName;
			Archive >> Value;
			IntParam[NameHandle(ParamName)] = Value;
		}

		// Deserialize float parameters
		uint32 FloatParamCount = 0;
		Archive >> FloatParamCount;
		FloatParam.clear();
		for (uint32 i = 0; i < FloatParamCount; ++i)
		{
			String ParamName;
			float Value;
			Archive >> ParamName;
			Archive >> Value;
			FloatParam[NameHandle(ParamName)] = Value;
		}

		// Deserialize color parameters
		uint32 ColorParamCount = 0;
		Archive >> ColorParamCount;
		ColorParam.clear();
		for (uint32 i = 0; i < ColorParamCount; ++i)
		{
			String ParamName;
			TVector4f Value;
			Archive >> ParamName;
			Archive >> Value.X >> Value.Y >> Value.Z >> Value.W;
			ColorParam[NameHandle(ParamName)] = Value;
		}

		// Deserialize texture parameters
		uint32 TextureParamCount = 0;
		Archive >> TextureParamCount;
		TextureParam.clear();
		for (uint32 i = 0; i < TextureParamCount; ++i)
		{
			String ParamName;
			TGuid Value;
			Archive >> ParamName;
			Archive >> Value;
			TextureParam[NameHandle(ParamName)] = Value;
		}
	}

	// Parameter setters
	void Material::SetIntParameter(const NameHandle& ParamName, int32 Value)
	{
		IntParam[ParamName] = Value;
	}

	void Material::SetFloatParameter(const NameHandle& ParamName, float Value)
	{
		FloatParam[ParamName] = Value;
	}

	void Material::SetColorParameter(const NameHandle& ParamName, const TVector4f& Value)
	{
		ColorParam[ParamName] = Value;
	}

	void Material::SetTextureParameter(const NameHandle& ParamName, const TGuid& TextureGuid)
	{
		TextureParam[ParamName] = TextureGuid;
		// Add texture GUID to dependencies
		AddDependency(TextureGuid);
	}

	// Parameter getters
	bool Material::GetIntParameter(const NameHandle& ParamName, int32& OutValue) const
	{
		auto It = IntParam.find(ParamName);
		if (It != IntParam.end())
		{
			OutValue = It->second;
			return true;
		}
		return false;
	}

	bool Material::GetFloatParameter(const NameHandle& ParamName, float& OutValue) const
	{
		auto It = FloatParam.find(ParamName);
		if (It != FloatParam.end())
		{
			OutValue = It->second;
			return true;
		}
		return false;
	}

	bool Material::GetColorParameter(const NameHandle& ParamName, TVector4f& OutValue) const
	{
		auto It = ColorParam.find(ParamName);
		if (It != ColorParam.end())
		{
			OutValue = It->second;
			return true;
		}
		return false;
	}

	bool Material::GetTextureParameter(const NameHandle& ParamName, TGuid& OutTextureGuid) const
	{
		auto It = TextureParam.find(ParamName);
		if (It != TextureParam.end())
		{
			OutTextureGuid = It->second;
			return true;
		}
		return false;
	}

	// Parameter removal
	void Material::RemoveIntParameter(const NameHandle& ParamName)
	{
		IntParam.erase(ParamName);
	}

	void Material::RemoveFloatParameter(const NameHandle& ParamName)
	{
		FloatParam.erase(ParamName);
	}

	void Material::RemoveColorParameter(const NameHandle& ParamName)
	{
		ColorParam.erase(ParamName);
	}

	void Material::RemoveTextureParameter(const NameHandle& ParamName)
	{
		TextureParam.erase(ParamName);
	}
}
