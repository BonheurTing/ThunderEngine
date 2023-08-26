#pragma once
#include "Platform.h"

struct CORE_API FileHelper
{
	static bool LoadFileToString(const String& fileName, String& outString);
	static int TraverseFileFromFolder(const String& folderPath,  Array<String>& outFileNames);
	static int TraverseFileFromFolderWithFormat(const String& folderPath,  Array<String>& outFileNames, const String& fileFormat);

	// project path
	static String GetProjectRoot();
	static String GetEngineRoot();
	static String GetEngineShaderRoot();
};

int TMessageBox(void* handle, const char* text, const char* caption, uint32 type);