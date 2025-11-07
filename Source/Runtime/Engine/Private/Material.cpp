#include "Material.h"
#include "FileSystem/MemoryArchive.h"
#include "RenderMaterial.h"
#include "ShaderArchive.h"
#include "ShaderModule.h"

namespace Thunder
{
	// ========== IMaterial Implementation ==========

	void IMaterial::CacheResourceShadersForRendering()
	{
		ShaderArchive* archive = GetShaderArchive();
		if (archive)
		{
			// TODO: 触发Shader编译
			// ShaderModule可能需要编译特定的Pass和Variant
		}
	}

	void IMaterial::BuildMaterialShaderMap()
	{
		// 生成ShaderMapId，调度异步编译
		// 根据材质参数和ShaderArchive生成唯一的ShaderMapId
		// TODO: 实现ShaderMapId生成和异步编译调度
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

			// 设置ShaderArchive
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

		return ShaderModule::GetModule()->GetShaderArchive(ArchiveName);
	}

	// ========== GameMaterial Implementation ==========

	GameMaterial::GameMaterial(GameObject* inOuter)
		: IMaterial(inOuter)
		, OverrideParameters(new MaterialParameterCache())
	{
	}

	GameMaterial::~GameMaterial()
	{
		if (OverrideParameters)
		{
			delete OverrideParameters;
			OverrideParameters = nullptr;
		}
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
		uint32 intParamCount = static_cast<uint32>(OverrideParameters->IntParameters.size());
		archive << intParamCount;
		for (const auto& pair : OverrideParameters->IntParameters)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}

		// Serialize float parameters
		uint32 floatParamCount = static_cast<uint32>(OverrideParameters->FloatParameters.size());
		archive << floatParamCount;
		for (const auto& pair : OverrideParameters->FloatParameters)
		{
			archive << pair.first.ToString();
			archive << pair.second;
		}

		// Serialize vector parameters
		uint32 vectorParamCount = static_cast<uint32>(OverrideParameters->VectorParameters.size());
		archive << vectorParamCount;
		for (const auto& pair : OverrideParameters->VectorParameters)
		{
			archive << pair.first.ToString();
			archive << pair.second.X << pair.second.Y << pair.second.Z << pair.second.W;
		}

		// Serialize texture parameters (as GUIDs)
		uint32 textureParamCount = static_cast<uint32>(OverrideParameters->TextureParameters.size());
		archive << textureParamCount;
		for (const auto& pair : OverrideParameters->TextureParameters)
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

		// Deserialize int parameters
		uint32 intParamCount = 0;
		archive >> intParamCount;
		OverrideParameters->IntParameters.clear();
		for (uint32 i = 0; i < intParamCount; ++i)
		{
			String paramName;
			int32 value;
			archive >> paramName;
			archive >> value;
			OverrideParameters->IntParameters[NameHandle(paramName)] = value;
		}

		// Deserialize float parameters
		uint32 floatParamCount = 0;
		archive >> floatParamCount;
		OverrideParameters->FloatParameters.clear();
		for (uint32 i = 0; i < floatParamCount; ++i)
		{
			String paramName;
			float value;
			archive >> paramName;
			archive >> value;
			OverrideParameters->FloatParameters[NameHandle(paramName)] = value;
		}

		// Deserialize vector parameters
		uint32 vectorParamCount = 0;
		archive >> vectorParamCount;
		OverrideParameters->VectorParameters.clear();
		for (uint32 i = 0; i < vectorParamCount; ++i)
		{
			String paramName;
			TVector4f value;
			archive >> paramName;
			archive >> value.X >> value.Y >> value.Z >> value.W;
			OverrideParameters->VectorParameters[NameHandle(paramName)] = value;
		}

		// Deserialize texture parameters
		uint32 textureParamCount = 0;
		archive >> textureParamCount;
		OverrideParameters->TextureParameters.clear();
		for (uint32 i = 0; i < textureParamCount; ++i)
		{
			String paramName;
			TGuid value;
			archive >> paramName;
			archive >> value;
			OverrideParameters->TextureParameters[NameHandle(paramName)] = value;
		}
	}

	// ========== Parameter setters ==========

	void GameMaterial::SetIntParameter(const NameHandle& paramName, int32 value)
	{
		OverrideParameters->IntParameters[paramName] = value;
		MarkRenderStateDirty();
	}

	void GameMaterial::SetFloatParameter(const NameHandle& paramName, float value)
	{
		OverrideParameters->FloatParameters[paramName] = value;
		MarkRenderStateDirty();
	}

	void GameMaterial::SetVectorParameter(const NameHandle& paramName, const TVector4f& value)
	{
		OverrideParameters->VectorParameters[paramName] = value;
		MarkRenderStateDirty();
	}

	void GameMaterial::SetTextureParameter(const NameHandle& paramName, const TGuid& textureGuid)
	{
		OverrideParameters->TextureParameters[paramName] = textureGuid;
		// Add texture GUID to dependencies
		AddDependency(textureGuid);
		MarkRenderStateDirty();
	}

	// ========== Parameter getters ==========

	bool GameMaterial::GetIntParameter(const NameHandle& paramName, int32& outValue) const
	{
		auto it = OverrideParameters->IntParameters.find(paramName);
		if (it != OverrideParameters->IntParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool GameMaterial::GetFloatParameter(const NameHandle& paramName, float& outValue) const
	{
		auto it = OverrideParameters->FloatParameters.find(paramName);
		if (it != OverrideParameters->FloatParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool GameMaterial::GetVectorParameter(const NameHandle& paramName, TVector4f& outValue) const
	{
		auto it = OverrideParameters->VectorParameters.find(paramName);
		if (it != OverrideParameters->VectorParameters.end())
		{
			outValue = it->second;
			return true;
		}
		return false;
	}

	bool GameMaterial::GetTextureParameter(const NameHandle& paramName, TGuid& outTextureGuid) const
	{
		auto it = OverrideParameters->TextureParameters.find(paramName);
		if (it != OverrideParameters->TextureParameters.end())
		{
			outTextureGuid = it->second;
			return true;
		}
		return false;
	}

	// ========== Parameter removal ==========

	void GameMaterial::RemoveIntParameter(const NameHandle& paramName)
	{
		OverrideParameters->IntParameters.erase(paramName);
		MarkRenderStateDirty();
	}

	void GameMaterial::RemoveFloatParameter(const NameHandle& paramName)
	{
		OverrideParameters->FloatParameters.erase(paramName);
		MarkRenderStateDirty();
	}

	void GameMaterial::RemoveVectorParameter(const NameHandle& paramName)
	{
		OverrideParameters->VectorParameters.erase(paramName);
		MarkRenderStateDirty();
	}

	void GameMaterial::RemoveTextureParameter(const NameHandle& paramName)
	{
		OverrideParameters->TextureParameters.erase(paramName);
		MarkRenderStateDirty();
	}

	void GameMaterial::UpdateRenderResource()
	{
		if (!DefaultRenderResource)
		{
			InitRenderResource();
		}

		if (DefaultRenderResource && bRenderStateDirty)
		{
			// 将GameThread的参数同步到RenderThread
			DefaultRenderResource->CacheParameters(OverrideParameters);
			bRenderStateDirty = false;
		}
	}
}
