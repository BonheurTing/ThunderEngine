#pragma once
#include "GameObject.h"
#include "NameHandle.h"
#include "Guid.h"
#include "Vector.h"

namespace Thunder
{
	class RenderMaterial;
	class ShaderArchive;
	struct ShaderParameterMap;

	class IMaterial : public GameResource
	{
	public:
		IMaterial(GameObject* inOuter = nullptr)
			: GameResource(inOuter, ETempGameResourceReflective::Material) {}
		virtual ~IMaterial() = default;

		void CacheResourceShadersForRendering();
		void BuildMaterialShaderMap();
		RenderMaterial* GetMaterialResource() const { return DefaultRenderResource; }
		void MarkRenderStateDirty() { bRenderStateDirty = true; }
		bool IsRenderStateDirty() const { return bRenderStateDirty; }
		virtual void UpdateRenderResource();
		void InitRenderResource();
		void ReleaseRenderResource();

		// ========== Shader Archive ==========
		void SetShaderArchive(NameHandle inArchive) { ArchiveName = inArchive; }
		NameHandle GetShaderArchiveName() const { return ArchiveName; }
		ShaderArchive* GetShaderArchive() const;

	protected:
		NameHandle ArchiveName;
		RenderMaterial* DefaultRenderResource { nullptr };
		bool bRenderStateDirty { true };
	};

	/**
	 * Material class that contains shader parameters and shader archive reference (= UE MaterialInstance)
	 */
	class GameMaterial : public IMaterial
	{
	public:
		ENGINE_API GameMaterial(GameObject* inOuter = nullptr);
		ENGINE_API ~GameMaterial() override;

		ENGINE_API void OnResourceLoaded() override;

		ENGINE_API void Serialize(MemoryWriter& archive) override;
		ENGINE_API void DeSerialize(MemoryReader& archive) override;

		ENGINE_API void SetIntParameter(const NameHandle& paramName, int32 value);
		ENGINE_API void SetFloatParameter(const NameHandle& paramName, float value);
		ENGINE_API void SetVectorParameter(const NameHandle& paramName, const TVector4f& value);
		ENGINE_API void SetTextureParameter(const NameHandle& paramName, const TGuid& textureGuid);
		ENGINE_API void SetStaticParameter(const NameHandle& paramName, bool value);

		ENGINE_API bool GetIntParameter(const NameHandle& paramName, int32& outValue) const;
		ENGINE_API bool GetFloatParameter(const NameHandle& paramName, float& outValue) const;
		ENGINE_API bool GetVectorParameter(const NameHandle& paramName, TVector4f& outValue) const;
		ENGINE_API bool GetTextureParameter(const NameHandle& paramName, TGuid& outTextureGuid) const;
		ENGINE_API bool GetStaticParameter(const NameHandle& paramName, bool& outValue) const;

		ENGINE_API void RemoveIntParameter(const NameHandle& paramName);
		ENGINE_API void RemoveFloatParameter(const NameHandle& paramName);
		ENGINE_API void RemoveVectorParameter(const NameHandle& paramName);
		ENGINE_API void RemoveTextureParameter(const NameHandle& paramName);

		// ========== Sync RenderThread ==========
		ENGINE_API void UpdateRenderResource() override;

		//const MaterialParameterCache* GetParameterCache() const { return OverrideParameters; }
		ENGINE_API void ResetDefaultParameters() const;

	private:
		ShaderParameterMap* ShaderParameters;
	};
}
