#include "FileSystem/FileModule.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

namespace Thunder
{
	IMPLEMENT_MODULE(File, FileModule)
	
	void FileModule::StartUp()
	{
		const String projRootPath = GetProjectRoot();
		
		IFileSystem* contentFileSys = new NativeFileSystem();
		contentFileSys->Mount(projRootPath + "\\Content\\");
		FileSystems.emplace("Content", contentFileSys);

		IFileSystem* savedFileSys = new NativeFileSystem();
		savedFileSys->Mount(projRootPath + "\\Saved\\");
		FileSystems.emplace("Saved", savedFileSys);
	}

	void FileModule::ShutDown()
	{
		
	}

	IFileSystemRef FileModule::GetFileSystem(const NameHandle& name)
	{
		auto& fileSystemMap = GetModule()->FileSystems;
		const auto iter = fileSystemMap.find(name);
		if (iter != fileSystemMap.end())
		{
			return iter->second;
		}
		return nullptr;
	}

	String FileModule::GetEngineRoot()
	{
		const String filePathBuffer = ".\\..\\..\\";
		char outPath[1024]="";
#if THUNDER_WINDOWS
		_fullpath(outPath, filePathBuffer.c_str(), 1024);
#else
		realpath(filePathBufffer, dir);
#endif

		return outPath;
	}

	String FileModule::GetProjectRoot()
	{
		//todo
		auto currentPath = std::filesystem::current_path();
		return currentPath.parent_path().parent_path().string();
	}

	String FileModule::GetEngineShaderRoot()
	{
		return GetProjectRoot() + "\\Shader\\";
	}

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
    
	bool FileModule::LoadFileToString(const String& fileName, String& outString)
	{
		std::ifstream fin;
		fin.open(fileName, std::ios::in);
		if (!fin.is_open())
		{
			LOG( "File Not Found!");
			return false;
		}
		std::ostringstream buf;
		char cn;
		while (buf && fin.get(cn))
		{
			buf.put(cn);
		}
		fin.close();
		outString = buf.str();
		return true;
	}

	bool FileModule::SaveFileFromString(const String& fileName, const String& inString)
	{
		std::ofstream fout;
		fout.open(fileName, std::ios::out);
		if (!fout.is_open())
		{
			LOG( "File Not Found!");
			return false;
		}
		fout << inString;
		fout.close();
		return true;
	}

	int FileModule::TraverseFileFromFolder(const String& folderPath, TArray<String>& outFileNames)
	{
		return TraverseFileFromFolderWithFormat(folderPath, outFileNames, "");
	}
    
	int FileModule::TraverseFileFromFolderWithFormat(const String& folderPath, TArray<String>& outFileNames, const String& fileFormat)
	{
		for (const auto& dirEntry : recursive_directory_iterator(folderPath))
		{
			String filePath = dirEntry.path().string();
			if (fileFormat.empty() || !filePath.ends_with(fileFormat))
				continue;
			outFileNames.push_back(filePath);
		}
    	
		return static_cast<int>(outFileNames.size());
	}
}
