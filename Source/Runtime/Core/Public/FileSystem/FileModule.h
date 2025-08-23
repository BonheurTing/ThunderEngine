#pragma once
#include "FileSystem.h"
#include "Module/ModuleManager.h"

namespace Thunder
{
	class CORE_API FileModule : public IModule
	{
		DECLARE_MODULE(File, FileModule)

	public:
		~FileModule() override = default;
		void StartUp() override;
		void ShutDown() override;
		static IFileSystemRef GetFileSystem(const NameHandle& name);

		static String GetEngineRoot();
		static String GetProjectRoot();
		static String GetEngineShaderRoot();
		static String GetFileExtension(const String& fileName);
		static String SwitchFileExtension(const String& fileName, const String& newExtension);
		static bool LoadFileToString(const String& fileName, String& outString);
		static bool SaveFileFromString(const String& fileName, const String& inString);
		static int TraverseFileFromFolder(const String& folderPath,  TArray<String>& outFileNames);
		static int TraverseFileFromFolderWithFormat(const String& folderPath,  TArray<String>& outFileNames, const String& fileFormat);

	private:
		TMap<NameHandle, IFileSystemRef> FileSystems;
	};
}

