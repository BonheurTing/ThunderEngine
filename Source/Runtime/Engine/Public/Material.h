#pragma once
#include "GameObject.h"
#include "NameHandle.h"
#include "Guid.h"
#include "Vector.h"

namespace Thunder
{
	class RenderMaterial;
	class ShaderArchive;
	class MaterialParameterCache;

	class IMaterial : public GameResource
	{
	public:
		IMaterial(GameObject* inOuter = nullptr)
			: GameResource(inOuter, ETempGameResourceReflective::Material)
			, DefaultRenderResource(nullptr)
			, bRenderStateDirty(true) {}
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
		RenderMaterial* DefaultRenderResource;
		bool bRenderStateDirty;
	};

	/**
	 * Material class that contains shader parameters and shader archive reference (= UE MaterialInstance)
	 */
	class ENGINE_API GameMaterial : public IMaterial
	{
	public:
		GameMaterial(GameObject* inOuter = nullptr);
		~GameMaterial() override;

		void OnResourceLoaded() override;

		void Serialize(MemoryWriter& archive) override;
		void DeSerialize(MemoryReader& archive) override;

		void SetIntParameter(const NameHandle& paramName, int32 value);
		void SetFloatParameter(const NameHandle& paramName, float value);
		void SetVectorParameter(const NameHandle& paramName, const TVector4f& value);
		void SetTextureParameter(const NameHandle& paramName, const TGuid& textureGuid);

		bool GetIntParameter(const NameHandle& paramName, int32& outValue) const;
		bool GetFloatParameter(const NameHandle& paramName, float& outValue) const;
		bool GetVectorParameter(const NameHandle& paramName, TVector4f& outValue) const;
		bool GetTextureParameter(const NameHandle& paramName, TGuid& outTextureGuid) const;

		void RemoveIntParameter(const NameHandle& paramName);
		void RemoveFloatParameter(const NameHandle& paramName);
		void RemoveVectorParameter(const NameHandle& paramName);
		void RemoveTextureParameter(const NameHandle& paramName);

		// ========== Sync RenderThread ==========
		void UpdateRenderResource() override;

		const MaterialParameterCache* GetParameterCache() const { return OverrideParameters; }

	private:
		MaterialParameterCache* OverrideParameters;
	};
}
