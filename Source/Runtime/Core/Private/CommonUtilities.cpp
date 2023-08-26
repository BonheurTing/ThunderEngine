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
	if (str.compare("True") == 0 || str.compare("true") == 0)
	{
		return true;
	}
	if (str.compare("False") == 0 || str.compare("false") == 0)
	{
		return false;
	}
	TAssertf(false, "CommonUtilities::StringToBool: Invaild input %s", str);
	return fallback;
}
