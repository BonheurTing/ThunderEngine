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
		const auto currentPath = std::filesystem::current_path();
		return currentPath.parent_path().parent_path().string();
	}

	String FileModule::GetEngineShaderRoot()
	{
		return GetProjectRoot() + "\\Shader\\";
	}

	String FileModule::GetResourceContentRoot()
	{
		return GetProjectRoot() + "\\Content\\";
	}

	String FileModule::GetFileName(const String& filePath)
	{
		// find lash '/' or '\\'
		size_t slashPosWin = filePath.find_last_of('\\');
		if (slashPosWin == String::npos)
		{
			slashPosWin = 0;
		}
		size_t slashPosUnix = filePath.find_last_of('/');
		if (slashPosUnix == String::npos)
		{
			slashPosUnix = 0;
		}
		const size_t slashPos = std::max(slashPosWin, slashPosUnix);

		// split file name
		String fileName;
		if (slashPos > 0 && slashPos < filePath.length()) {
			fileName = filePath.substr(slashPos + 1);
		} else {
			fileName = filePath; // if no '/', file name is whole name
		}
		
		// find last '.'
		const size_t dotPos = fileName.find_last_of('.');
		if (dotPos != String::npos) {
			return fileName.substr(0, dotPos); // return substr before '.'
		}
		
		return fileName;
	}

	String FileModule::GetFileExtension(const String& filePath)
	{
		const size_t dotPos = filePath.find_last_of('.');
    
		// find lash '/' or '\\'
		size_t slashPosWin = filePath.find_last_of('\\');
		if (slashPosWin == String::npos)
		{
			slashPosWin = 0;
		}
		size_t slashPosUnix = filePath.find_last_of('/');
		if (slashPosUnix == String::npos)
		{
			slashPosUnix = 0;
		}
		const size_t slashPos = std::max(slashPosWin, slashPosUnix);
    
		// 如果找到了点号，并且点号在最后一个路径分隔符之后（即确实是文件扩展名）
		if (dotPos != std::string::npos && dotPos > slashPos) {
			// 返回点号之后的所有字符（即文件扩展名）
			return filePath.substr(dotPos + 1);
		}
    
		// 如果没有找到有效的扩展名，返回空字符串
		return "";
	}

	String FileModule::SwitchFileExtension(const String& filePath, const String& newExtension)
	{
		const size_t dotPos = filePath.find_last_of('.');
    
		// 查找最后一个路径分隔符的位置（支持Windows和Unix风格）
		const size_t slashPos = std::max(
			filePath.find_last_of('/'),  // Unix风格分隔符
			filePath.find_last_of('\\')  // Windows风格分隔符
		);
    
		// 如果找到了点号，并且点号在最后一个路径分隔符之后（即确实是文件扩展名）
		if (dotPos != std::string::npos && (slashPos == std::string::npos || dotPos > slashPos)) {
			// 替换扩展名：取点号之前的部分 + 新扩展名
			const String baseName = filePath.substr(0, dotPos);
			if (newExtension.empty() || newExtension[0] == '.') {
				return baseName + newExtension;
			} else {
				return baseName + "." + newExtension;
			}
		}
    
		// 如果没有找到有效的扩展名，直接添加新扩展名
		if (newExtension.empty() || newExtension[0] == '.') {
			return filePath + newExtension;
		} else {
			return filePath + "." + newExtension;
		}
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
			if (!fileFormat.empty() && !filePath.ends_with(fileFormat))
				continue;
			outFileNames.push_back(filePath);
		}
    	
		return static_cast<int>(outFileNames.size());
	}
}
