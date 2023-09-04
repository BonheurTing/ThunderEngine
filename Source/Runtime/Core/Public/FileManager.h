#pragma once
#include "Container.h"
#include "Platform.h"

namespace Thunder
{
	struct CORE_API FileManager
	{
		bool LoadFileToString(const String& fileName, String& outString);
		int TraverseFileFromFolder(const String& folderPath,  Array<String>& outFileNames);
		int TraverseFileFromFolderWithFormat(const String& folderPath,  Array<String>& outFileNames, const String& fileFormat);

		// project path
		String GetProjectRoot();
		String GetEngineRoot();
		String GetEngineShaderRoot();
	};

	int TMessageBox(void* handle, const char* text, const char* caption, uint32 type);
}