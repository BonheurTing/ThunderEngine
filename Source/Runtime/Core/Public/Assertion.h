﻿#pragma once

#define CA_ASSUME( Expr )
#define check(expr)					{ CA_ASSUME(expr); }
#include <cstdio>
#include "CoreMinimal.h"
#include "FileHelper.h"

namespace Thunder
{
	namespace Core
	{
		template<typename... Types>
		void ProcessAssertion(const char* expression, const char* file, int line, const char* format, const Types&... args)
		{
			char assertMessage[4096];
			sprintf(assertMessage, format, args...);

			char messageBoxBuf[65535];
			sprintf(messageBoxBuf, ("Assertion failed \"%s\"\nFile: \"%s\"\nLine: %d\n%s\n"), expression, file, line, assertMessage);

			const int result = TMessageBox(nullptr, &messageBoxBuf[0], "Assert Failed", MB_OKCANCEL);
#if _WIN64
			if (result == IDOK)
			{
				DebugBreak();
			}
#endif
		}
	}
}

#define TAssert(expr) assert(expr)
#define TAssertf(expr, ...)  if(!expr) Thunder::Core::ProcessAssertion(#expr, __FILE__, __LINE__, __VA_ARGS__)