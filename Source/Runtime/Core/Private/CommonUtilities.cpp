#include "CommonUtilities.h"
#include "Assertion.h"

int CommonUtilities::StringToInteger(const String& str, int fallback)
{
	if (str.length() > 0)
	{
		return atoi(str.c_str());
	}
	return fallback;
}

bool CommonUtilities::StringToBool(const String& str, bool fallback)
{
	if (str == "True"  || str =="true")
	{
		return true;
	}
	if (str == "False" || str == "false")
	{
		return false;
	}
	TAssertf(str.empty(), "CommonUtilities::StringToBool: Invaild input %s", str);
	return fallback;
}
