#pragma once
#include "Platform.h"

struct CORE_API FileHelper
{
	static bool LoadFileToString(const String& fileName, String& outString);
	static int TraverseFileFromFolder(const String& folderPath,  Array<String>& outFileNames);
	static int TraverseFileFromFolderWithFormat(const String& folderPath,  Array<String>& outFileNames, const String& fileFormat);

	// project path
	static String GetProjectPath();
};
