#pragma optimize("", off)

#include <cstdio>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;


extern void yyerror(const char* s);
extern void ThunderParse(const char* str);

int main(int argc, char* argv[])
{
  fs::path current_path = fs::current_path();
  std::string str = current_path.string() + "/../../Shader/example.tsf";
  std::cout << "解析shader文件: " << str << std::endl;
  
  ThunderParse(str.c_str());
  fflush(stdout);
  return 0;
}


#pragma optimize("", on)

