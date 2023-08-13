#include "FileHelper.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <direct.h>

using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

bool FileHelper::LoadFileToString(const String& fileName, String& outString)
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

int FileHelper::TraverseFileFromFolder(const String& folderPath, Array<String>& outFileNames)
{
	return TraverseFileFromFolderWithFormat(folderPath, outFileNames, "");
}

int FileHelper::TraverseFileFromFolderWithFormat(const String& folderPath, Array<String>& outFileNames, const String& fileFormat)
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

String FileHelper::GetProjectPath()
{
	char buffer[64];
	_getcwd(buffer, 64);
	return buffer;
}
