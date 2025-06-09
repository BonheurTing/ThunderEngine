#include "ShaderLang.h"
#include "AstNode.h"

namespace Thunder
{

	void DebugLog(shader_lang_state* state, enum_debug_level debugLevel, const char* message, int lineNo)
	{
		printf("%s", message);
	}
}
