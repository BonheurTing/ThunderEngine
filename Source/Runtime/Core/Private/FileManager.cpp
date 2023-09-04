#include "FileManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include "CoreMinimal.h"

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
    
    int FileManager::TraverseFileFromFolder(const String& folderPath, Array<String>& outFileNames)
    {
    	return TraverseFileFromFolderWithFormat(folderPath, outFileNames, "");
    }
    
    int FileManager::TraverseFileFromFolderWithFormat(const String& folderPath, Array<String>& outFileNames, const String& fileFormat)
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
	    return "";
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
    	return GetEngineRoot() + "\\Shader";
    }
}
