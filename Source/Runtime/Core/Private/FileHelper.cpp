#include "FileHelper.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

bool FFileHelper::LoadFileToString(const String& filename, String& outString)
{
	std::ifstream fin;
	fin.open(filename, std::ios::in);
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
