#include "Config/ConfigManager.h"
#include "Assertion.h"
#include "CoreModule.h"
#include "FileManager.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

namespace Thunder
{
	using namespace rapidjson;

	bool ConfigManager::LoadConfig(NameHandle configName)
	{
		if (ConfigContainerList.contains(configName))
		{
			return true;
		}
		return ReLoadConfig(configName);
	}

	bool ConfigManager::ReLoadConfig(NameHandle configName)
	{
		String configContent;
		const String configPath = GFileManager->GetEngineRoot() + "Config/" + configName.ToString() + ".json";
		if (!GFileManager->LoadFileToString(configPath, configContent))
		{
			LOG("Fail to load config file to string: %s", configPath.c_str());
			return false;
		}
		Document document;
		document.Parse(configContent.c_str());
		const ParseErrorCode errorCode = document.GetParseError();
		TAssertf(document.IsObject(), "Parse json file error.\nError code: %d\nFile name: \"%s\"", errorCode, configName.c_str());

		const auto container = new ConfigDataContainer{};
		uint32 configDataOffset = 0;
		// build layout
		for (auto iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
		{
			ConfigMember member;
			NameHandle memberName = iter->name.GetString();
			member.Offset = configDataOffset;
			if (iter->value.IsBool())
			{
				member.Type = EConfigType::Boolean;
				configDataOffset += sizeof(bool);
			}
			else if (iter->value.IsInt() || iter->value.IsFloat())
			{
				member.Type = EConfigType::Float;
				configDataOffset += sizeof(float);
			}
			else if (iter->value.IsString())
			{
				member.Type = EConfigType::String;
				configDataOffset = configDataOffset + iter->value.GetStringLength() + 1;
			}
			else
			{
				TAssertf(false, "Parse config member error.\nError code: %d\nFile name: \"%s\"\nMember name: \"%s\"", errorCode, configName.c_str(), memberName);
			}
			container->Layout[memberName] = member;
		}
		// build data
		container->Data = static_cast<uint8*>(TMemory::Malloc<uint8>(configDataOffset));
		for (auto iter : container->Layout)
		{
			NameHandle memberName = iter.first;
			auto configData = document.FindMember(memberName.ToString().c_str());
			switch (iter.second.Type)
			{
				case EConfigType::Boolean:
				{
					bool configValue = configData->value.GetBool();
					memcpy(container->Data + container->Layout[memberName].Offset, &configValue, sizeof(bool));
					break;
				}
				case EConfigType::Float:
				{
					float configValue = configData->value.IsInt() ? static_cast<float>(configData->value.GetInt()) : configData->value.GetFloat();
					memcpy(container->Data + container->Layout[memberName].Offset, &configValue, sizeof(float));
					break;
				}
				case EConfigType::String:
				{
					const auto configValue = configData->value.GetString();
					memcpy(container->Data + container->Layout[memberName].Offset, configValue, configData->value.GetStringLength());
					container->Data[container->Layout[memberName].Offset + configData->value.GetStringLength()] = '\0';
					break;
				}	
			}
		}
		ConfigContainerList[configName] = container;
		LOG("Succeed to load config file: %s", configName.c_str());
		return true;
	}

	bool ConfigManager::SaveConfig(NameHandle configName)
	{
		if (ConfigContainerList.contains(configName))
		{
			ConfigDataContainer* container = ConfigContainerList[configName];
			Document document;
			document.SetObject();
			for (auto iter : container->Layout)
			{
				const ConfigMember& member = iter.second;
				if (member.Type == EConfigType::Boolean)
				{
					document.AddMember(Value(iter.first.c_str(), document.GetAllocator()), Value(container->GetBool(iter.first)), document.GetAllocator());
				}
				else if (member.Type == EConfigType::Float)
				{
					document.AddMember(Value(iter.first.c_str(), document.GetAllocator()), Value(container->GetFloat(iter.first)), document.GetAllocator());
				}
				else if (member.Type == EConfigType::String)
				{
					String str = container->GetString(iter.first);
					document.AddMember(Value(iter.first.c_str(), document.GetAllocator()), Value(str.c_str(), str.size(), document.GetAllocator()), document.GetAllocator());
				}
			}
			StringBuffer buffer;
			Writer<StringBuffer> writer{buffer};
			document.Accept(writer);
			const String configContent = buffer.GetString();
			GFileManager->SaveFileFromString(configName.ToString(), configContent);
			return true;
		}
		return false;
	}

	ConfigDataContainer* ConfigManager::GetConfig(NameHandle name, bool force/* = true*/)
	{
		if (LoadConfig(name))
		{
			TAssertf(ConfigContainerList.contains(name) || !force, "Config not found: %s", name.c_str());
			return ConfigContainerList.contains(name) ? ConfigContainerList[name] : nullptr;
		}
		return nullptr;
	}

	ConfigMember* ConfigManager::GetConfigMember(NameHandle containerName, EConfigType configType, NameHandle configName)
	{
		if (ConfigContainerList.contains(containerName))
		{
			ConfigDataContainer* container = ConfigContainerList[containerName];
			if (container->Layout.contains(configName))
			{
				ConfigMember& member = container->Layout[configName];
				if (member.Type == configType)
				{
					return &member;
				}
			}
		}
		return nullptr;
	}
}
