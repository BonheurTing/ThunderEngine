#pragma optimize("", off)

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "PreProcessor.h"
#include "Module/ModuleManager.h"
#include "CoreModule.h"
#include "FileSystem/FileModule.h"
#include "ShaderLang.h"

namespace Thunder
{
	class CoreModule;
}

namespace fs = std::filesystem;

extern Thunder::shader_lang_state* ThunderParse(const char* text);

int main(int argc, char* argv[])
{
	Thunder::ModuleManager::GetInstance()->LoadModule<Thunder::CoreModule>();
	std::string str = Thunder::FileModule::GetEngineShaderRoot() +  "/example.tsf";
	std::cout << "解析shader文件: " << str << std::endl;

	std::ifstream file(str);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string raw_text = buffer.str();
	Thunder::String processed_text = Thunder::PreProcessor::Process(raw_text).c_str();;
	Thunder::shader_lang_state* st = ThunderParse(processed_text.c_str());
	fflush(stdout);

	Thunder::ModuleManager::GetInstance()->UnloadModule<Thunder::CoreModule>();
	return 0;
}



