#pragma optimize("", off)
#include "Material.h"
#include "FileSystem/MemoryArchive.h"
#include "RenderMaterial.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"
#include "ShaderParameterMap.h"
#include "Concurrent/TaskScheduler.h"

namespace Thunder
{
	// ========== IMaterial Implementation ==========

	void IMaterial::CacheResourceShadersForRendering()
	{
	}

	void IMaterial::BuildMaterialShaderMap()
	{
	}

	void IMaterial::UpdateRenderResource()
	{
		if (DefaultRenderResource && bRenderStateDirty)
		{
			bRenderStateDirty = false;
		}
	}

	void IMaterial::InitRenderResource()
	{
		if (!DefaultRenderResource)
		{
			DefaultRenderResource = new RenderMaterial();

			ShaderArchive* archive = GetShaderArchive();
			if (archive)
			{
				DefaultRenderResource->SetShaderArchive(archive);
			}
		}
	}

	void IMaterial::ReleaseRenderResource()
	{
		if (DefaultRenderResource)
		{
			delete DefaultRenderResource;
			DefaultRenderResource = nullptr;
		}
	}

	ShaderArchive* IMaterial::GetShaderArchive() const
	{
		if (ArchiveName.IsEmpty())
		{
			return nullptr;
		}

		return ShaderModule::GetShaderArchive(ArchiveName);
	}

	// ========== GameMaterial Implementation ==========

	GameMaterial::GameMaterial(GameObject* inOuter)
		: IMaterial(inOuter)
		, ShaderParameters(new ShaderParameterMap())
	{
	}

	GameMaterial::~GameMaterial()
	{
	}

	void GameMaterial::OnResourceLoaded()
	{
		LOG("Material loaded: %s", GetResourceName().c_str());

		UpdateRenderResource();
	}

	void GameMaterial::Serialize(MemoryWriter& archive)
	{
		IMaterial::Serialize(archive);

		// Serialize shader archive name
		archive << ArchiveName.ToString();

		// Serialize int parameters
		uint32 intParamCount = static_cast<uint32>(ShaderParameters->IntParameters.size());
		archive << intParamCount;
		for (const auto& pair : ShaderParameters->IntParameters)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}

		// Serialize float parameters
		uint32 floatParamCount = static_cast<uint32>(ShaderParameters->FloatParameters.size());
		archive << floatParamCount;
		for (const auto& pair : ShaderParameters->FloatParameters)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}

		// Serialize vector parameters
		uint32 vectorParamCount = static_cast<uint32>(ShaderParameters->VectorParameters.size());
		archive << vectorParamCount;
		for (const auto& pair : ShaderParameters->VectorParameters)
		{
			archive << pair.first.ToString();
			archive << pair.second.X << pair.second.Y << pair.second.Z << pair.second.W;
		}

		// Serialize texture parameters (as GUIDs)
		uint32 textureParamCount = static_cast<uint32>(ShaderParameters->TextureParameters.size());
		archive << textureParamCount;
		for (const auto& pair : ShaderParameters->TextureParameters)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}

		// Serialize virant parameters
		uint32 boolParamCount = static_cast<uint32>(ShaderParameters->StaticSwitchParameters.size());
		archive << boolParamCount;
		for (const auto& pair : ShaderParameters->StaticSwitchParameters)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}
	}

	void GameMaterial::DeSerialize(MemoryReader& archive)
	{
		IMaterial::DeSerialize(archive);

		// Deserialize shader archive name
		String archiveName;
		archive >> archiveName;
		ArchiveName = NameHandle(archiveName);
		ResetDefaultParameters();

		// Deserialize int parameters
		uint32 intParamCount = 0;
		archive >> intParamCount;
		// OverrideParameters->IntParameters.clear();
		for (uint32 i = 0; i < intParamCount; ++i)
		{
			String paramName;
			int32 value;
			archive >> paramName;
			archive >> value;
			ShaderParameters->IntParameters[NameHandle(paramName)] = value;
		}

		// Deserialize float parameters
		uint32 floatParamCount = 0;
		archive >> floatParamCount;
		// OverrideParameters->FloatParameters.clear();
		for (uint32 i = 0; i < floatParamCount; ++i)
		{
			String paramName;
			float value;
			archive >> paramName;
			archive >> value;
			ShaderParameters->FloatParameters[NameHandle(paramName)] = value;
		}

		// Deserialize vector parameters
		uint32 vectorParamCount = 0;
		archive >> vectorParamCount;
		// OverrideParameters->VectorParameters.clear();
		for (uint32 i = 0; i < vectorParamCount; ++i)
		{
			String paramName;
			TVector4f value;
			archive >> paramName;
			archive >> value.X >> value.Y >> value.Z >> value.W;
			ShaderParameters->VectorParameters[NameHandle(paramName)] = value;
		}

		// Deserialize texture parameters
		uint32 textureParamCount = 0;
		archive >> textureParamCount;
		// OverrideParameters->TextureParameters.clear();
		for (uint32 i = 0; i < textureParamCount; ++i)
		{
			String paramName;
			TGuid value;
			archive >> paramName;
			archive >> value;
			ShaderParameters->TextureParameters[NameHandle(paramName)] = value;
		}

		// Deserialize float parameters
		uint32 boolParamCount = 0;
		archive >> boolParamCount;
		// OverrideParameters->StaticParameters.clear();
		for (uint32 i = 0; i < boolParamCount; ++i)
		{
			String paramName;
			bool value;
			archive >> paramName;
			archive >> value;
			ShaderParameters->StaticSwitchParameters[NameHandle(paramName)] = value;
		}
	}

	// ========== Parameter setters ==========

	void GameMaterial::SetIntParameter(const NameHandle& paramName, int32 value)
	{
		ShaderParameters->IntParameters[paramName] = value;
		MarkRenderStateDirty();
	}

	void GameMaterial::SetFloatParameter(const NameHandle& paramName, float value)
	{
		ShaderParameters->FloatParameters[paramName] = value;
		MarkRenderStateDirty();
	}

	void GameMaterial::SetVectorParameter(const NameHandle& paramName, const TVector4f& value)
	{
		ShaderParameters->VectorParameters[paramName] = value;
		MarkRenderStateDirty();
	}

	void GameMaterial::SetTextureParameter(const NameHandle& paramName, const TGuid& textureGuid)
	{
		ShaderParameters->TextureParameters[paramName] = textureGuid;
		AddDependency(textureGuid);
		MarkRenderStateDirty();
	}

	void GameMaterial::SetStaticParameter(const NameHandle& paramName, bool value)
	{
		ShaderParameters->StaticSwitchParameters[paramName] = value;
		MarkRenderStateDirty();
	}

	// ========== Parameter getters ==========

	bool GameMaterial::GetIntParameter(const NameHandle& paramName, int32& outValue) const
	{
		auto it = ShaderParameters->IntParameters.find(paramName);
		if (it != ShaderParameters->IntParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool GameMaterial::GetFloatParameter(const NameHandle& paramName, float& outValue) const
	{
		auto it = ShaderParameters->FloatParameters.find(paramName);
		if (it != ShaderParameters->FloatParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool GameMaterial::GetVectorParameter(const NameHandle& paramName, TVector4f& outValue) const
	{
		auto it = ShaderParameters->VectorParameters.find(paramName);
		if (it != ShaderParameters->VectorParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool GameMaterial::GetTextureParameter(const NameHandle& paramName, TGuid& outTextureGuid) const
	{
		auto it = ShaderParameters->TextureParameters.find(paramName);
		if (it != ShaderParameters->TextureParameters.end())
		{
			outTextureGuid = it->second;
			return true;
		}
		return false;
	}

	bool GameMaterial::GetStaticParameter(const NameHandle& paramName, bool& outValue) const
	{
		auto it = ShaderParameters->StaticSwitchParameters.find(paramName);
		if (it != ShaderParameters->StaticSwitchParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	// ========== Parameter removal ==========

	void GameMaterial::RemoveIntParameter(const NameHandle& paramName)
	{
		ShaderParameters->IntParameters.erase(paramName);
		MarkRenderStateDirty();
	}

	void GameMaterial::RemoveFloatParameter(const NameHandle& paramName)
	{
		ShaderParameters->FloatParameters.erase(paramName);
		MarkRenderStateDirty();
	}

	void GameMaterial::RemoveVectorParameter(const NameHandle& paramName)
	{
		ShaderParameters->VectorParameters.erase(paramName);
		MarkRenderStateDirty();
	}

	void GameMaterial::RemoveTextureParameter(const NameHandle& paramName)
	{
		ShaderParameters->TextureParameters.erase(paramName);
		MarkRenderStateDirty();
	}

	// Todo : a update list is needed.
	void GameMaterial::UpdateRenderResource()
	{
		if (!DefaultRenderResource)
		{
			InitRenderResource();
		}

		if (DefaultRenderResource && bRenderStateDirty)
		{
			GRenderScheduler->PushTask([this]()
			{
				DefaultRenderResource->CacheParameters(ShaderParameters);
				bRenderStateDirty = false;
			});
		}
	}

	void GameMaterial::ResetDefaultParameters() const
	{
		ShaderModule::GenerateDefaultParameters(ArchiveName, ShaderParameters);
	}
}

