#include "Platform.h"

#include "Assertion.h"

namespace Thunder
{
	int TMessageBox(void* handle, const char* text, const char* caption, uint32 type)
	{
#if _WIN64
		const int result = ::MessageBoxA(static_cast<HWND>(handle), text, caption, type);
		return result;
#else
		assert(false);
		return 1;
#endif
	}
}
