#pragma once
#include "CoreMinimal.h"
#include "CommonUtilities.h"

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
    	NameHandle EntryPoint;
    	uint64 VariantMask = 0;
    };
    
    struct ShaderStage
    {
    	BinaryData ByteCode;
    	uint64 VariantId;
    };
    
    struct ShaderCombination
    {
    	ShaderCombination() = default;
		ShaderCombination(const ShaderCombination& rhs) = default;
		ShaderCombination& operator=(const ShaderCombination& rhs) = default;
    	ShaderCombination& operator=(ShaderCombination&& rhs) noexcept
    	{
    		Shaders = std::move(rhs.Shaders);
    		return *this;
    	}
    	ShaderCombination(ShaderCombination&& rhs) noexcept
    	{
    		Shaders = std::move(rhs.Shaders);
    	}
    	
    	HashMap<EShaderStageType, ShaderStage> Shaders;
    };
    
    struct VariantMeta
    {
    	NameHandle Name;
    	bool Default;
    	bool Fallback;
    	bool Visible;
    	NameHandle Texture;
    
    	VariantMeta() {Default = false; Fallback = false; Visible = false;}
    	VariantMeta(const String& name, const String& defaultValue = "", const String& fallback = "",
    	            const String& visible = "",const String& texture = "") : Name(name), Texture(texture)
    	{
    		Default = CommonUtilities::StringToBool(defaultValue, false);
    		Fallback = CommonUtilities::StringToBool(fallback, false);
    		Visible = CommonUtilities::StringToBool(visible, true);
    	}
    
    	bool operator==(const VariantMeta& value) const
    	{
    		return Name == value.Name
    		&& Default == value.Default
    		&& Fallback == value.Fallback
    		&& Visible == value.Visible
    		&& Texture == value.Texture;
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

