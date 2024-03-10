#pragma once
#include "Container.h"
#include "Templates/RefCountObject.h"

namespace Thunder
{
	struct CORE_API FileManager : public RefCountedObject
	{
		bool LoadFileToString(const String& fileName, String& outString);
		bool SaveFileFromString(const String& fileName, const String& inString);
		int TraverseFileFromFolder(const String& folderPath,  TArray<String>& outFileNames);
		int TraverseFileFromFolderWithFormat(const String& folderPath,  TArray<String>& outFileNames, const String& fileFormat);

		// project path
		String GetProjectRoot() const;
		String GetEngineRoot() const;
		String GetEngineShaderRoot() const;
	};
}