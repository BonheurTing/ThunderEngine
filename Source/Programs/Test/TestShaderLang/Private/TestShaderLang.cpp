#pragma optimize("", off)

#include <cstdio>
#include <cstring>
#include <iostream>

extern void yyerror(const char* s);
extern void ThunderParse(const char* str);

int main(int argc, char* argv[])
{
  ThunderParse("D:/ThunderEngine/Shader/example.tsf");
  fflush(stdout);
  return 0;
}


#pragma optimize("", on)

