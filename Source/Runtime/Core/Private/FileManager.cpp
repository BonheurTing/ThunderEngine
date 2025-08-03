#include "FileManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

namespace Thunder
{
	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
    
    bool FileManager::LoadFileToString(const String& fileName, String& outString)
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

    bool FileManager::SaveFileFromString(const String& fileName, const String& inString)
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

    int FileManager::TraverseFileFromFolder(const String& folderPath, TArray<String>& outFileNames)
    {
    	return TraverseFileFromFolderWithFormat(folderPath, outFileNames, "");
    }
    
    int FileManager::TraverseFileFromFolderWithFormat(const String& folderPath, TArray<String>& outFileNames, const String& fileFormat)
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
    
    String FileManager::GetProjectRoot() const
    {
    	//todo
    	auto currentPath = std::filesystem::current_path();
	    return currentPath.parent_path().parent_path().parent_path().parent_path().parent_path().string();
    }
    
    String FileManager::GetEngineRoot() const
    {
    	const String filePathBuffer = ".\\..\\..\\";
    	char outPath[1024]="";
#if _WIN32 || _WIN64
    	_fullpath(outPath, filePathBuffer.c_str(), 1024);
#else
    	realpath(filePathBufffer, dir);
#endif

    	return outPath;
    }
    
    String FileManager::GetEngineShaderRoot() const
    {
    	return GetProjectRoot() + "\\Shader\\";
    }
}
