#pragma once
#include "CoreMinimal.h"
#include "Guid.h"
#include "Vector.h"

namespace Thunder
{
	struct SHADER_API ShaderParameterMap
	{
		TMap<NameHandle, int32> IntParameters;
		TMap<NameHandle, float> FloatParameters;
		TMap<NameHandle, TVector4f> VectorParameters;
		TMap<NameHandle, TGuid> TextureParameters;
		TMap<NameHandle, bool> StaticSwitchParameters;

		ShaderParameterMap() = default;

		ShaderParameterMap(const ShaderParameterMap& other)
		{
			if (this == &other)
			{
				return;
			}
			IntParameters = other.IntParameters;
			FloatParameters = other.FloatParameters;
			VectorParameters = other.VectorParameters;
			TextureParameters = other.TextureParameters;
			StaticSwitchParameters = other.StaticSwitchParameters;
		}

		ShaderParameterMap(ShaderParameterMap&& other) noexcept
		{
			if (this == &other)
			{
				return;
			}
			IntParameters = std::move(other.IntParameters);
			FloatParameters = std::move(other.FloatParameters);
			VectorParameters = std::move(other.VectorParameters);
			TextureParameters = std::move(other.TextureParameters);
			StaticSwitchParameters = std::move(other.StaticSwitchParameters);
		}

		ShaderParameterMap& operator=(const ShaderParameterMap& other)
		{
			if (this != &other)
			{
				IntParameters = other.IntParameters;
				FloatParameters = other.FloatParameters;
				VectorParameters = other.VectorParameters;
				TextureParameters = other.TextureParameters;
				StaticSwitchParameters = other.StaticSwitchParameters;
			}
			return *this;
		}

		ShaderParameterMap& operator=(ShaderParameterMap&& other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}
			IntParameters = std::move(other.IntParameters);
			FloatParameters = std::move(other.FloatParameters);
			VectorParameters = std::move(other.VectorParameters);
			TextureParameters = std::move(other.TextureParameters);
			StaticSwitchParameters = std::move(other.StaticSwitchParameters);
			return *this;
		}

		void Reset();

		void SetIntParameter(const NameHandle& paramName, int32 value);
		void SetFloatParameter(const NameHandle& paramName, float value);
		void SetVectorParameter(const NameHandle& paramName, const TVector4f& value);
		void SetTextureParameter(const NameHandle& paramName, const TGuid& textureGuid);
		void SetTextureParameter(const NameHandle& paramName, uint32 textureId);
		void SetStaticSwitchParameter(const NameHandle& paramName, bool value);

		bool GetIntParameter(const NameHandle& paramName, int32& outValue) const;
		bool GetFloatParameter(const NameHandle& paramName, float& outValue) const;
		bool GetVectorParameter(const NameHandle& paramName, TVector4f& outValue) const;
		bool GetTextureParameter(const NameHandle& paramName, TGuid& outTextureGuid) const;
		bool GetStaticSwitchParameter(const NameHandle& paramName, bool& outValue) const;

		void RemoveIntParameter(const NameHandle& paramName);
		void RemoveFloatParameter(const NameHandle& paramName);
		void RemoveVectorParameter(const NameHandle& paramName);
		void RemoveTextureParameter(const NameHandle& paramName);
		void RemoveStaticSwitchParameter(const NameHandle& paramName);
	};
}

