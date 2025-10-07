#pragma once
#include "GameObject.h"
#include "ShaderArchive.h"
#include "NameHandle.h"
#include "Container.h"
#include "Guid.h"
#include "Vector.h"

namespace Thunder
{
	class IMaterial : public GameResource
	{
	public:
		IMaterial(GameObject* InOuter = nullptr)
			: GameResource(InOuter, ETempGameResourceReflective::Material) {}
		virtual ~IMaterial() = default;
	};

	/**
	 * Material class that contains shader parameters and shader archive reference
	 */
	class ENGINE_API Material : public IMaterial
	{
	public:
		Material(GameObject* InOuter = nullptr);
		~Material() override = default;

		// Resource lifecycle
		void OnResourceLoaded() override;

		// Serialization
		void Serialize(MemoryWriter& Archive) override;
		void DeSerialize(MemoryReader& Archive) override;

		// Shader archive
		void SetShaderArchive(class ShaderArchive* InArchive) { Archive = InArchive; }
		ShaderArchive* GetShaderArchive() const { return Archive; }

		// Parameter setters
		void SetIntParameter(const NameHandle& ParamName, int32 Value);
		void SetFloatParameter(const NameHandle& ParamName, float Value);
		void SetColorParameter(const NameHandle& ParamName, const TVector4f& Value);
		void SetTextureParameter(const NameHandle& ParamName, const TGuid& TextureGuid);

		// Parameter getters
		bool GetIntParameter(const NameHandle& ParamName, int32& OutValue) const;
		bool GetFloatParameter(const NameHandle& ParamName, float& OutValue) const;
		bool GetColorParameter(const NameHandle& ParamName, TVector4f& OutValue) const;
		bool GetTextureParameter(const NameHandle& ParamName, TGuid& OutTextureGuid) const;

		// Parameter removal
		void RemoveIntParameter(const NameHandle& ParamName);
		void RemoveFloatParameter(const NameHandle& ParamName);
		void RemoveColorParameter(const NameHandle& ParamName);
		void RemoveTextureParameter(const NameHandle& ParamName);

		// Get all parameters
		const TMap<NameHandle, int32>& GetIntParameters() const { return IntParam; }
		const TMap<NameHandle, float>& GetFloatParameters() const { return FloatParam; }
		const TMap<NameHandle, TVector4f>& GetColorParameters() const { return ColorParam; }
		const TMap<NameHandle, TGuid>& GetTextureParameters() const { return TextureParam; }

	private:
		class ShaderArchive* Archive { nullptr };

		// Material parameters
		TMap<NameHandle, int32> IntParam;
		TMap<NameHandle, float> FloatParam;
		TMap<NameHandle, TVector4f> ColorParam;  // Using Vector4 for Color (RGBA)
		TMap<NameHandle, TGuid> TextureParam;    // Using GUID for texture references
	};
}
