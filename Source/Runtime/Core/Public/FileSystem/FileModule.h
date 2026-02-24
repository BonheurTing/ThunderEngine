#pragma once
#include "FileSystem.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
	class FileModule : public IModule
	{
		DECLARE_MODULE(File, FileModule, CORE_API)

	public:
		CORE_API ~FileModule() override = default;
		CORE_API void StartUp() override;
		CORE_API void ShutDown() override;
		CORE_API static IFileSystemRef GetFileSystem(const NameHandle& name);

		CORE_API static String GetEngineRoot();
		CORE_API static String GetProjectRoot();
		CORE_API static String GetEngineShaderRoot();
		CORE_API static String GetResourceContentRoot();
		CORE_API static String GetFileName(const String& filePath);
		CORE_API static String GetFileExtension(const String& filePath);
		CORE_API static String SwitchFileExtension(const String& filePath, const String& newExtension);
		CORE_API static bool LoadFileToString(const String& fileName, String& outString);
		CORE_API static bool SaveFileFromString(const String& fileName, const String& inString);
		CORE_API static int TraverseFileFromFolder(const String& folderPath,  TArray<String>& outFileNames);
		CORE_API static int TraverseFileFromFolderWithFormat(const String& folderPath,  TArray<String>& outFileNames, const String& fileFormat);

	private:
		TMap<NameHandle, IFileSystemRef> FileSystems;
	};
}

