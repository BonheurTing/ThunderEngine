#pragma once
#include "BinaryData.h"
#include "CoreMinimal.h"
#include "CommonUtilities.h"
#include "Guid.h"
#include "Vector.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
	enum class EShaderStageType : uint8
    {
    	Unknown	= 0,
    	Vertex,
    	Hull,
    	Domain,
    	Pixel,
    	Compute,
		Geometry,
    	Mesh
    };

    struct StageMeta
    {
    	String EntryPoint;
    	uint64 VariantMask = 0;
    };

	struct ShaderBytecodeHash
	{
		uint64 Hash[2];

		bool operator ==(const ShaderBytecodeHash& b) const
		{
			return (Hash[0] == b.Hash[0] && Hash[1] == b.Hash[1]);
		}

		bool operator !=(const ShaderBytecodeHash& b) const
		{
			return (Hash[0] != b.Hash[0] || Hash[1] != b.Hash[1]);
		}
	};
    
    struct ShaderStage : public RefCountedObject
    {
    	BinaryData ByteCode;
    	uint64 VariantId;

    	ShaderBytecodeHash GetBytecodeHash() const; //todo only d3d
    };
	using ShaderStageRef = TRefCountPtr<ShaderStage, true>;
    
    struct ShaderCombination : public RefCountedObject
    {
    	ShaderCombination() = default;
		/*ShaderCombination(const ShaderCombination& rhs) = default;
		ShaderCombination& operator=(const ShaderCombination& rhs) = default;*/
    	ShaderCombination& operator=(ShaderCombination&& rhs) noexcept
    	{
    		Shaders = std::move(rhs.Shaders);
    		return *this;
    	}
    	ShaderCombination(ShaderCombination&& rhs) noexcept
    	{
    		Shaders = std::move(rhs.Shaders);
    	}
    	~ShaderCombination() override
    	{
    		for (auto& pair : Shaders)
    		{
    			if (pair.second->ByteCode.Size > 0)
    			{
    				TMemory::Free(pair.second->ByteCode.Data);
    			}
    		}
    	}

    	BinaryData* GetByteCode(EShaderStageType type)
    	{
	        const auto iter = Shaders.find(type);
    		if (iter != Shaders.end())
			{
				return &(iter->second->ByteCode);
			}
    		return nullptr;
    	}

    	static uint32 GetTypeHash(const ShaderCombination& combination);

    	THashMap<EShaderStageType, ShaderStageRef> Shaders;
    };

    using ShaderCombinationRef = TRefCountPtr<ShaderCombination, true>;
    
    struct ShaderVariantMeta
    {
    	NameHandle Name;
    	bool Default;
    	bool Fallback;
    	bool Visible;
    
    	ShaderVariantMeta() {Default = false; Fallback = false; Visible = false;}
    	ShaderVariantMeta(const String& name, const String& defaultValue = "", const String& fallback = "",
    	            const String& visible = "",const String& texture = "") : Name(name)
    	{
    		Default = CommonUtilities::StringToBool(defaultValue, false);
    		Fallback = CommonUtilities::StringToBool(fallback, false);
    		Visible = CommonUtilities::StringToBool(visible, true);
    	}
    
    	bool operator==(const ShaderVariantMeta& value) const
    	{
    		return Name == value.Name
    		&& Default == value.Default
    		&& Fallback == value.Fallback
    		&& Visible == value.Visible;
    	}
    };

    /*namespace std
    {
    	template<>
    	struct hash<VariantMeta>
    	{
    		size_t operator()(const VariantMeta& value) const
    		{
    			return hash<String>()(value.Name);
    		}
    	};
    }*/
    
    struct ShaderPropertyMeta
    {
    	NameHandle Name;
    	String DisplayName;
    	String Type;
    	String Format;
    	String Default;
    	String Range;
    };
    
    struct ShaderParameterMeta
    {
    	NameHandle Name;
    	String Type;
    	String Format;
    	String Default;
    };
    
    struct RenderStateMeta
    {
    	String Blend;
    	String Cull;
    	int LOD;
    };
    
    enum class ECompilePriority : uint8
    {
    	CheckCache	= 0,
    	ForceLocal,
    	Max
    };

	struct MaterialParameterCache
	{
		TMap<NameHandle, int32> IntParameters;
		TMap<NameHandle, float> FloatParameters;
		TMap<NameHandle, TVector4f> VectorParameters;
		TMap<NameHandle, TGuid> TextureParameters;
		TMap<NameHandle, bool> StaticParameters;

		MaterialParameterCache() = default;

		MaterialParameterCache(const MaterialParameterCache& other)
		{
			if (this == &other)
			{
				return;
			}
			IntParameters = other.IntParameters;
			FloatParameters = other.FloatParameters;
			VectorParameters = other.VectorParameters;
			TextureParameters = other.TextureParameters;
			StaticParameters = other.StaticParameters;
		}

		MaterialParameterCache(MaterialParameterCache&& other) noexcept
		{
			if (this == &other)
			{
				return;
			}
			IntParameters = std::move(other.IntParameters);
			FloatParameters = std::move(other.FloatParameters);
			VectorParameters = std::move(other.VectorParameters);
			TextureParameters = std::move(other.TextureParameters);
			StaticParameters = std::move(other.StaticParameters);
		}

		MaterialParameterCache& operator=(const MaterialParameterCache& other)
		{
			if (this != &other)
			{
				IntParameters = other.IntParameters;
				FloatParameters = other.FloatParameters;
				VectorParameters = other.VectorParameters;
				TextureParameters = other.TextureParameters;
				StaticParameters = other.StaticParameters;
			}
			return *this;
		}

		MaterialParameterCache& operator=(MaterialParameterCache&& other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}
			IntParameters = std::move(other.IntParameters);
			FloatParameters = std::move(other.FloatParameters);
			VectorParameters = std::move(other.VectorParameters);
			TextureParameters = std::move(other.TextureParameters);
			StaticParameters = std::move(other.StaticParameters);
			return *this;
		}
	};
}

