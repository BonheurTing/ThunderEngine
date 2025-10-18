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
		IMaterial(GameObject* inOuter = nullptr)
			: GameResource(inOuter, ETempGameResourceReflective::Material) {}
		virtual ~IMaterial() = default;
	};

	/**
	 * Material class that contains shader parameters and shader archive reference
	 */
	class ENGINE_API Material : public IMaterial
	{
	public:
		Material(GameObject* inOuter = nullptr);
		~Material() override = default;

		// Resource lifecycle
		void OnResourceLoaded() override;

		// Serialization
		void Serialize(MemoryWriter& archive) override;
		void DeSerialize(MemoryReader& archive) override;

		// Shader archive
		void SetShaderArchive(class ShaderArchive* inArchive) { Archive = inArchive; }
		ShaderArchive* GetShaderArchive() const { return Archive; }

		// Parameter setters
		void SetIntParameter(const NameHandle& paramName, int32 value);
		void SetFloatParameter(const NameHandle& paramName, float value);
		void SetColorParameter(const NameHandle& paramName, const TVector4f& value);
		void SetTextureParameter(const NameHandle& paramName, const TGuid& textureGuid);

		// Parameter getters
		bool GetIntParameter(const NameHandle& paramName, int32& outValue) const;
		bool GetFloatParameter(const NameHandle& paramName, float& outValue) const;
		bool GetColorParameter(const NameHandle& paramName, TVector4f& outValue) const;
		bool GetTextureParameter(const NameHandle& paramName, TGuid& outTextureGuid) const;

		// Parameter removal
		void RemoveIntParameter(const NameHandle& paramName);
		void RemoveFloatParameter(const NameHandle& paramName);
		void RemoveColorParameter(const NameHandle& paramName);
		void RemoveTextureParameter(const NameHandle& paramName);

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
