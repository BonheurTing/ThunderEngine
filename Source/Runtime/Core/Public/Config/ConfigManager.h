#pragma once
#include "Container.h"
#include "NameHandle.h"
#include "Memory/MemoryBase.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
	enum class EConfigType : uint8
	{
		Boolean = 0,
		Float,
		String,
		Num
	};
	
	struct ConfigMember
	{
		uint32 Offset = 0;
		EConfigType Type = EConfigType::Num;
		//void* Bind = nullptr;

		_NODISCARD_ bool IsBool() const { return Type == EConfigType::Boolean; }
		_NODISCARD_ bool IsFloat() const { return Type == EConfigType::Float; }
		_NODISCARD_ bool IsString() const { return Type == EConfigType::String; }
	};
	
	struct ConfigDataContainer
	{
		uint8* Data = nullptr;
		TMap<NameHandle, ConfigMember> Layout;

		~ConfigDataContainer()
		{
			if (Data)
			{
				TMemory::Destroy(Data);
			}
		}
		bool GetBool(NameHandle name)
		{
			return *reinterpret_cast<bool*>(Data + Layout[name].Offset);
		}
		float GetFloat(NameHandle name)
		{
			return *reinterpret_cast<float*>(Data + Layout[name].Offset);
		}
		int GetFloatAsInt(NameHandle name)
		{
			return static_cast<int>(GetFloat(name));
		}
		String GetString(NameHandle name)
		{
			String str(reinterpret_cast<char*>(Data + Layout[name].Offset));
			return str;
		}
	};

	class ConfigManager : public RefCountedObject
	{
	public:
		CORE_API bool LoadConfig(NameHandle configName);
		CORE_API bool ReLoadConfig(NameHandle configName);
		CORE_API bool SaveConfig(NameHandle configName);

		CORE_API ConfigDataContainer* GetConfig(NameHandle name, bool force = true);
		CORE_API ConfigMember* GetConfigMember(NameHandle containerName, EConfigType configType, NameHandle configName);

	private:
		TMap<NameHandle, ConfigDataContainer*> ConfigContainerList;
	};
}
