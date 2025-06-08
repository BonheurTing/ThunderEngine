#pragma optimize("", off)

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "ShaderLang.h"

namespace fs = std::filesystem;

extern void ThunderParse(const char* text);

int main(int argc, char* argv[])
{
	fs::path current_path = fs::current_path();
	std::string str = "D:/ThunderEngine/Shader/example.tsf";
	std::cout << "解析shader文件: " << str << std::endl;

	std::ifstream file(str);
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string text = buffer.str();
	//const char* text = shaderCode.c_str();
	ThunderParse(text.c_str());
	fflush(stdout);
	return 0;
}


#pragma optimize("", on)

