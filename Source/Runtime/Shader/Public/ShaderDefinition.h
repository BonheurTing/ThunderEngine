#pragma once
#include "BinaryData.h"
#include "CoreMinimal.h"
#include "CommonUtilities.h"
#include "Guid.h"
#include "Vector.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
	#define MAX_SRVS		32
	#define MAX_SAMPLERS	8
	#define MAX_UAVS		8
	#define MAX_CBS			8

	enum class EShaderStageType : uint8
    {
    	Unknown	= 0,
    	Vertex,
    	Hull,
    	Domain,
    	Pixel,
    	Compute,
		Geometry,
    	Mesh,
		Num
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
    
    class ShaderCombination : public RefCountedObject
    {
    public:
    	ShaderCombination(class ShaderPass* subShader) : SubShader(subShader) {}
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
    	
    	ShaderPass* GetSubShader() const { return SubShader; }
    	THashMap<EShaderStageType, ShaderStageRef>& GetShaders() { return Shaders; }

    private:
    	ShaderPass* SubShader = nullptr;
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
}

