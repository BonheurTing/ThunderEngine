#pragma once
#include "Container.h"

namespace Thunder
{
	struct CORE_API FileManager
	{
		bool LoadFileToString(const String& fileName, String& outString);
		int TraverseFileFromFolder(const String& folderPath,  Array<String>& outFileNames);
		int TraverseFileFromFolderWithFormat(const String& folderPath,  Array<String>& outFileNames, const String& fileFormat);

		// project path
		String GetProjectRoot() const;
		String GetEngineRoot() const;
		String GetEngineShaderRoot() const;
	};
}