#include "ShaderModule.h"
#include "FileHelper.h"
#include "rapidjson/document.h"

using namespace rapidjson;

bool ShaderModule::ParseShaderFile()
{
	Array<String> shaderNameList;
	FileHelper::TraverseFileFromFolderWithFormat("D:\\ThunderEngine\\Shader", shaderNameList, "shader");
	
	for (const String& meta: shaderNameList)
	{
		String metaContent;
		LOG("%s", meta.c_str());
		if (!FileHelper::LoadFileToString(meta, metaContent))
		{
			continue;
		}
		Document document;
		document.Parse(metaContent.c_str());
		assert(document.IsObject());
		assert(document.HasMember("Name"));
		NameHandle newShaderArchiveName = document["Name"].GetString();
		
		
	}
	if(!shaderNameList.empty())
	{
		return true;
	}
	return false;
}